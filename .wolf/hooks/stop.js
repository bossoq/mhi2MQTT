import * as fs from "node:fs";
import * as path from "node:path";
import { getWolfDir, ensureWolfDir, readJSON, writeJSON, appendMarkdown, timeShort } from "./shared.js";
async function main() {
    ensureWolfDir();
    const wolfDir = getWolfDir();
    const hooksDir = path.join(wolfDir, "hooks");
    const sessionFile = path.join(hooksDir, "_session.json");
    const session = readJSON(sessionFile, {
        session_id: "",
        started: "",
        files_read: {},
        files_written: [],
        edit_counts: {},
        anatomy_hits: 0,
        anatomy_misses: 0,
        repeated_reads_warned: 0,
        cerebrum_warnings: 0,
        stop_count: 0,
    });
    session.stop_count++;
    // Only write to ledger if there's been activity
    const readCount = Object.keys(session.files_read).length;
    const writeCount = session.files_written.length;
    if (readCount === 0 && writeCount === 0) {
        writeJSON(sessionFile, session);
        process.exit(0);
        return;
    }
    // The Stop hook fires at the end of every turn, but files_read/files_written
    // accumulate across the whole session and are never reset. Without this
    // guard, every turn re-appends an identical "Session end" row to memory.md
    // and re-pushes the same entry into the ledger, inflating lifetime totals.
    // Do nothing unless something actually changed since the last Stop.
    const inputTokens = Object.values(session.files_read).reduce((sum, r) => sum + r.tokens, 0);
    const outputTokens = session.files_written.reduce((sum, w) => sum + w.tokens, 0);
    // Estimated savings: anatomy hits save ~200 tokens each, repeated reads
    // save their token count for each read after the first.
    const savedFromAnatomy = session.anatomy_hits * 200;
    const savedFromRepeats = Object.values(session.files_read)
        .filter((r) => r.count > 1)
        .reduce((sum, r) => sum + r.tokens * (r.count - 1), 0);
    // Every metric that feeds a lifetime delta below must appear here, or a
    // change to one of them alone (e.g. a repeated read bumps repeatsBlocked
    // and savings without moving readCount or the token totals) would be
    // swallowed by the guard and never reach the ledger.
    const checkpoint = {
        readCount,
        writeCount,
        inputTokens,
        outputTokens,
        anatomyHits: session.anatomy_hits,
        anatomyMisses: session.anatomy_misses,
        repeatsBlocked: session.repeated_reads_warned,
        savings: savedFromAnatomy + savedFromRepeats,
    };
    const prev = session.reported;
    if (prev && Object.keys(checkpoint).every((k) => prev[k] === checkpoint[k])) {
        writeJSON(sessionFile, session);
        process.exit(0);
        return;
    }
    // Check for files edited many times without a buglog entry
    checkForMissingBugLogs(wolfDir, session);
    // Check if cerebrum was updated this session (it should be if there were edits)
    checkCerebrumFreshness(wolfDir, session);
    // Build session entry for ledger
    const reads = Object.entries(session.files_read).map(([file, data]) => ({
        file,
        tokens_estimated: data.tokens,
        was_repeated: data.count > 1,
        anatomy_had_description: false, // simplified
    }));
    const writes = session.files_written.map((w) => ({
        file: w.file,
        tokens_estimated: w.tokens,
        action: w.action,
    }));
    const sessionEntry = {
        id: session.session_id,
        started: session.started,
        ended: new Date().toISOString(),
        reads,
        writes,
        totals: {
            input_tokens_estimated: inputTokens,
            output_tokens_estimated: outputTokens,
            reads_count: readCount,
            writes_count: writeCount,
            repeated_reads_blocked: session.repeated_reads_warned,
            anatomy_lookups: session.anatomy_hits,
        },
    };
    // Update token-ledger.json
    const ledgerPath = path.join(wolfDir, "token-ledger.json");
    const ledger = readJSON(ledgerPath, {
        version: 1,
        created_at: "",
        lifetime: {
            total_tokens_estimated: 0,
            total_reads: 0,
            total_writes: 0,
            total_sessions: 0,
            anatomy_hits: 0,
            anatomy_misses: 0,
            repeated_reads_blocked: 0,
            estimated_savings_vs_bare_cli: 0,
        },
        sessions: [],
        daemon_usage: [],
        waste_flags: [],
        optimization_report: { last_generated: null, patterns: [] },
    });
    // Replace this session's entry rather than appending a new one per turn.
    const existingIdx = ledger.sessions.findIndex((s) => s.id === session.session_id);
    if (existingIdx >= 0)
        ledger.sessions[existingIdx] = sessionEntry;
    else
        ledger.sessions.push(sessionEntry);
    // Apply only the delta since the last recorded Stop, so repeated Stops
    // within one session cannot inflate lifetime totals.
    ledger.lifetime.total_reads += readCount - (prev?.readCount ?? 0);
    ledger.lifetime.total_writes += writeCount - (prev?.writeCount ?? 0);
    ledger.lifetime.total_tokens_estimated += (inputTokens + outputTokens) - ((prev?.inputTokens ?? 0) + (prev?.outputTokens ?? 0));
    ledger.lifetime.anatomy_hits += session.anatomy_hits - (prev?.anatomyHits ?? 0);
    ledger.lifetime.anatomy_misses += session.anatomy_misses - (prev?.anatomyMisses ?? 0);
    ledger.lifetime.repeated_reads_blocked += session.repeated_reads_warned - (prev?.repeatsBlocked ?? 0);
    // Estimate savings: anatomy hits save ~200 tokens each, repeated reads blocked save their token count
    ledger.lifetime.estimated_savings_vs_bare_cli += (savedFromAnatomy + savedFromRepeats) - (prev?.savings ?? 0);
    writeJSON(ledgerPath, ledger);
    // Write a session summary line to memory.md if there was meaningful activity
    if (writeCount > 0) {
        try {
            const uniqueFiles = new Set(session.files_written.map(w => path.basename(w.file)));
            const fileList = [...uniqueFiles].slice(0, 5).join(", ");
            const memoryPath = path.join(wolfDir, "memory.md");
            const row = `| ${timeShort()} | Session end: ${writeCount} writes across ${uniqueFiles.size} files (${fileList}) | ${readCount} reads | ~${inputTokens + outputTokens} tok |`;
            // Rewrite this session's row instead of stacking a near-identical
            // one every turn. Only the row this session already wrote is
            // touched; earlier sessions' rows are left alone.
            let rewritten = false;
            if (session.memory_row_written) {
                const lines = fs.readFileSync(memoryPath, "utf8").split("\n");
                for (let i = lines.length - 1; i >= 0; i--) {
                    if (lines[i].includes("| Session end:")) {
                        lines[i] = row;
                        fs.writeFileSync(memoryPath, lines.join("\n"));
                        rewritten = true;
                        break;
                    }
                }
            }
            if (!rewritten) {
                appendMarkdown(memoryPath, row + "\n");
                session.memory_row_written = true;
            }
        }
        catch { }
    }
    session.reported = checkpoint;
    writeJSON(sessionFile, session);
    process.exit(0);
}
/**
 * Check if files were edited multiple times but buglog.json wasn't updated.
 * Emit a stderr reminder so Claude sees it in the next turn.
 */
function checkForMissingBugLogs(wolfDir, session) {
    if (!session.edit_counts)
        return;
    const multiEditFiles = Object.entries(session.edit_counts)
        .filter(([, count]) => count >= 3)
        .map(([file]) => path.basename(file));
    if (multiEditFiles.length === 0)
        return;
    // Check if buglog was written to this session
    const buglogWritten = session.files_written.some(w => w.file.includes("buglog.json"));
    if (!buglogWritten) {
        process.stderr.write(`⚠️ OpenWolf: Files edited 3+ times this session (${multiEditFiles.join(", ")}) but buglog.json was not updated. If you fixed bugs, please log them.\n`);
    }
}
/**
 * Check if cerebrum.md was updated recently. If it hasn't been updated in
 * a while and there was significant activity, emit a gentle reminder.
 */
function checkCerebrumFreshness(wolfDir, session) {
    const cerebrumPath = path.join(wolfDir, "cerebrum.md");
    try {
        const stat = fs.statSync(cerebrumPath);
        const hoursSinceUpdate = (Date.now() - stat.mtimeMs) / (1000 * 60 * 60);
        // If cerebrum hasn't been updated in 24h+ and there were significant writes
        if (hoursSinceUpdate > 24 && session.files_written.length >= 3) {
            process.stderr.write(`💡 OpenWolf: cerebrum.md hasn't been updated in ${Math.floor(hoursSinceUpdate)}h. Did you learn any user preferences, conventions, or gotchas this session? Consider updating .wolf/cerebrum.md.\n`);
        }
    }
    catch {
        // cerebrum.md doesn't exist, that's ok
    }
}
main().catch(() => process.exit(0));
//# sourceMappingURL=stop.js.map