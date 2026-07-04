#pragma once
// String <-> enum mappings between MQTT/web payloads and MHI driver enums.
// Hardware-free (only <string.h> + mhi-ac-types.h) so it compiles in
// native (host) unit tests. All *ToStr functions return static string
// literals — safe to store in HVACSettings without ownership tracking.
#include "MHI-AC-Ctrl/mhi-ac-types.h"

mhi_ac::ACPower strToPower(const char *s);
mhi_ac::ACMode strToMode(const char *s);
mhi_ac::ACFan strToFan(const char *s);
mhi_ac::ACVanesUD strToVaneUD(const char *s);
mhi_ac::ACVanesLR strToVaneLR(const char *s);

const char *powerToStr(mhi_ac::ACPower p);
const char *modeToStr(mhi_ac::ACMode m);
const char *fanToStr(mhi_ac::ACFan f);
const char *vaneUDToStr(mhi_ac::ACVanesUD v);
const char *vaneLRToStr(mhi_ac::ACVanesLR v);
