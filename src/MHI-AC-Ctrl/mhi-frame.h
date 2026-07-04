#pragma once
// Pure frame codec logic: signature/checksum validation, TX checksum
// generation, and temperature encode/decode. Hardware-free (only
// MHI-AC-Ctrl-internal.h + libc) so it compiles in native (host) unit
// tests. Callers are responsible for logging on validation failure.
#include <stdint.h>
#include <array>
#include "MHI-AC-Ctrl-internal.h"

namespace mhi_ac
{
    namespace frame
    {
        // Validation error codes returned by validate_frame
        constexpr int ERR_SIGNATURE = -1;      // bad SB0/SB1/SB2 signature
        constexpr int ERR_CHECKSUM_SHORT = -2; // bad 16-bit short-frame checksum
        constexpr int ERR_CHECKSUM_LONG = -3;  // bad 8-bit long-frame checksum

        bool validate_signature(uint8_t sb0, uint8_t sb1, uint8_t sb2);

        // Validates signature + checksum(s). frame_len must be
        // MHI_FRAME_LEN_SHORT or MHI_FRAME_LEN_LONG. Returns 0 on success
        // or one of the ERR_* codes above.
        int validate_frame(const std::array<uint8_t, MHI_FRAME_LEN_LONG> &mosi_frame, uint8_t frame_len);

        // Computes and writes CBH/CBL (short) and CBL2 (long) checksums.
        void apply_tx_checksums(std::array<uint8_t, MHI_FRAME_LEN_LONG> &frame);

        // Temperature codecs
        uint8_t target_temp_encode(float celsius);   // DB2 value incl. set bit
        float target_temp_decode(uint8_t db2);       // DB2 -> setpoint celsius
        float room_temp_decode(uint8_t db3);         // DB3 -> room celsius
        uint8_t room_temp_encode(float celsius);     // celsius -> DB3
    } // namespace frame
} // namespace mhi_ac
