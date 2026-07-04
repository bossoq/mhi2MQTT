#include <string.h>
#include "mhi_mappings.h"

mhi_ac::ACPower strToPower(const char *s) {
    return (strcasecmp(s, "ON") == 0) ? mhi_ac::ACPower::power_on : mhi_ac::ACPower::power_off;
}

mhi_ac::ACMode strToMode(const char *s) {
    if (strcasecmp(s, "HEAT") == 0)     return mhi_ac::ACMode::mode_heat;
    if (strcasecmp(s, "DRY") == 0)      return mhi_ac::ACMode::mode_dry;
    if (strcasecmp(s, "FAN") == 0)      return mhi_ac::ACMode::mode_fan;
    if (strcasecmp(s, "FAN_ONLY") == 0) return mhi_ac::ACMode::mode_fan;
    if (strcasecmp(s, "AUTO") == 0)     return mhi_ac::ACMode::mode_auto;
    if (strcasecmp(s, "HEAT_COOL") == 0) return mhi_ac::ACMode::mode_auto;
    return mhi_ac::ACMode::mode_cool;
}

mhi_ac::ACFan strToFan(const char *s) {
    if (strcasecmp(s, "AUTO") == 0)  return mhi_ac::ACFan::speed_auto;
    if (strcmp(s, "2") == 0)         return mhi_ac::ACFan::speed_2;
    if (strcmp(s, "3") == 0)         return mhi_ac::ACFan::speed_3;
    if (strcmp(s, "4") == 0 || strcmp(s, "5") == 0) return mhi_ac::ACFan::speed_4;
    return mhi_ac::ACFan::speed_1;
}

mhi_ac::ACVanesUD strToVaneUD(const char *s) {
    if (strcasecmp(s, "SWING") == 0) return mhi_ac::ACVanesUD::Swing;
    if (strcmp(s, "2") == 0)         return mhi_ac::ACVanesUD::UpCenter;
    if (strcmp(s, "3") == 0)         return mhi_ac::ACVanesUD::CenterDown;
    if (strcmp(s, "4") == 0)         return mhi_ac::ACVanesUD::Down;
    return mhi_ac::ACVanesUD::Up;
}

mhi_ac::ACVanesLR strToVaneLR(const char *s) {
    if (strcasecmp(s, "SWING") == 0)  return mhi_ac::ACVanesLR::Swing;
    if (strcasecmp(s, "WIDE") == 0)   return mhi_ac::ACVanesLR::Wide;
    if (strcasecmp(s, "SPOT") == 0)   return mhi_ac::ACVanesLR::Spot;
    if (strcmp(s, "2") == 0)          return mhi_ac::ACVanesLR::LeftCenter;
    if (strcmp(s, "3") == 0)          return mhi_ac::ACVanesLR::Center;
    if (strcmp(s, "4") == 0)          return mhi_ac::ACVanesLR::CenterRight;
    if (strcmp(s, "5") == 0)          return mhi_ac::ACVanesLR::Right;
    if (strcmp(s, "6") == 0)          return mhi_ac::ACVanesLR::Wide;
    return mhi_ac::ACVanesLR::Left;
}

const char *powerToStr(mhi_ac::ACPower p) {
    return (p == mhi_ac::ACPower::power_on) ? "ON" : "OFF";
}

const char *modeToStr(mhi_ac::ACMode m) {
    switch (m) {
        case mhi_ac::ACMode::mode_heat: return "HEAT";
        case mhi_ac::ACMode::mode_dry:  return "DRY";
        case mhi_ac::ACMode::mode_fan:  return "FAN";
        case mhi_ac::ACMode::mode_auto: return "AUTO";
        default:                        return "COOL";
    }
}

const char *fanToStr(mhi_ac::ACFan f) {
    switch (f) {
        case mhi_ac::ACFan::speed_2:    return "2";
        case mhi_ac::ACFan::speed_3:    return "3";
        case mhi_ac::ACFan::speed_4:    return "4";
        case mhi_ac::ACFan::speed_auto: return "AUTO";
        default:                        return "1";
    }
}

const char *vaneUDToStr(mhi_ac::ACVanesUD v) {
    switch (v) {
        case mhi_ac::ACVanesUD::Swing:       return "SWING";
        case mhi_ac::ACVanesUD::UpCenter:    return "2";
        case mhi_ac::ACVanesUD::CenterDown:  return "3";
        case mhi_ac::ACVanesUD::Down:        return "4";
        default:                             return "1";
    }
}

const char *vaneLRToStr(mhi_ac::ACVanesLR v) {
    switch (v) {
        case mhi_ac::ACVanesLR::Swing:       return "SWING";
        case mhi_ac::ACVanesLR::Wide:        return "WIDE";
        case mhi_ac::ACVanesLR::Spot:        return "SPOT";
        case mhi_ac::ACVanesLR::LeftCenter:  return "2";
        case mhi_ac::ACVanesLR::Center:      return "3";
        case mhi_ac::ACVanesLR::CenterRight: return "4";
        case mhi_ac::ACVanesLR::Right:       return "5";
        default:                             return "1";
    }
}
