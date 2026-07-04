// Native unit tests for the string <-> enum mapping helpers.
// Run: pio test -e native
#include <unity.h>
#include "../../src/mhi_mappings.cpp"

using namespace mhi_ac;

void setUp(void) {}
void tearDown(void) {}

// ---- power ----

static void test_power_round_trip(void)
{
    TEST_ASSERT_EQUAL_STRING("ON", powerToStr(strToPower("ON")));
    TEST_ASSERT_EQUAL_STRING("OFF", powerToStr(strToPower("OFF")));
}

static void test_power_case_insensitive(void)
{
    TEST_ASSERT_TRUE(strToPower("on") == ACPower::power_on);
    TEST_ASSERT_TRUE(strToPower("On") == ACPower::power_on);
}

static void test_power_unknown_defaults_off(void)
{
    TEST_ASSERT_TRUE(strToPower("banana") == ACPower::power_off);
    TEST_ASSERT_TRUE(strToPower("") == ACPower::power_off);
}

// ---- mode ----

static void test_mode_round_trip(void)
{
    const char *modes[] = {"HEAT", "DRY", "FAN", "AUTO", "COOL"};
    for (auto m : modes)
    {
        TEST_ASSERT_EQUAL_STRING(m, modeToStr(strToMode(m)));
    }
}

static void test_mode_aliases(void)
{
    TEST_ASSERT_TRUE(strToMode("FAN_ONLY") == ACMode::mode_fan);
    TEST_ASSERT_TRUE(strToMode("HEAT_COOL") == ACMode::mode_auto);
}

static void test_mode_case_insensitive(void)
{
    TEST_ASSERT_TRUE(strToMode("heat") == ACMode::mode_heat);
    TEST_ASSERT_TRUE(strToMode("auto") == ACMode::mode_auto);
}

static void test_mode_unknown_defaults_cool(void)
{
    TEST_ASSERT_TRUE(strToMode("banana") == ACMode::mode_cool);
    TEST_ASSERT_EQUAL_STRING("COOL", modeToStr(ACMode::mode_unknown));
}

// ---- fan ----

static void test_fan_round_trip(void)
{
    const char *fans[] = {"1", "2", "3", "4", "AUTO"};
    for (auto f : fans)
    {
        TEST_ASSERT_EQUAL_STRING(f, fanToStr(strToFan(f)));
    }
}

static void test_fan_5_maps_to_4(void)
{
    // Daikin had 5 speeds; MHI has 4 — "5" is a compatibility alias
    TEST_ASSERT_TRUE(strToFan("5") == ACFan::speed_4);
    TEST_ASSERT_EQUAL_STRING("4", fanToStr(strToFan("5")));
}

static void test_fan_unknown_defaults_1(void)
{
    TEST_ASSERT_TRUE(strToFan("banana") == ACFan::speed_1);
    TEST_ASSERT_EQUAL_STRING("1", fanToStr(ACFan::unknown));
}

// ---- vertical vane (up/down) ----

static void test_vane_ud_round_trip(void)
{
    const char *vanes[] = {"1", "2", "3", "4", "SWING"};
    for (auto v : vanes)
    {
        TEST_ASSERT_EQUAL_STRING(v, vaneUDToStr(strToVaneUD(v)));
    }
}

static void test_vane_ud_unknown_defaults_up(void)
{
    TEST_ASSERT_TRUE(strToVaneUD("banana") == ACVanesUD::Up);
    TEST_ASSERT_EQUAL_STRING("1", vaneUDToStr(ACVanesUD::SeeIRRemote));
}

// ---- horizontal vane (left/right) ----

static void test_vane_lr_round_trip(void)
{
    const char *vanes[] = {"1", "2", "3", "4", "5", "SWING", "WIDE", "SPOT"};
    for (auto v : vanes)
    {
        TEST_ASSERT_EQUAL_STRING(v, vaneLRToStr(strToVaneLR(v)));
    }
}

static void test_vane_lr_6_is_wide_alias(void)
{
    // "6" is accepted as an alias for WIDE but serializes back as "WIDE"
    TEST_ASSERT_TRUE(strToVaneLR("6") == ACVanesLR::Wide);
    TEST_ASSERT_EQUAL_STRING("WIDE", vaneLRToStr(strToVaneLR("6")));
}

static void test_vane_lr_unknown_defaults_left(void)
{
    TEST_ASSERT_TRUE(strToVaneLR("banana") == ACVanesLR::Left);
}

int main(int argc, char **argv)
{
    UNITY_BEGIN();
    RUN_TEST(test_power_round_trip);
    RUN_TEST(test_power_case_insensitive);
    RUN_TEST(test_power_unknown_defaults_off);
    RUN_TEST(test_mode_round_trip);
    RUN_TEST(test_mode_aliases);
    RUN_TEST(test_mode_case_insensitive);
    RUN_TEST(test_mode_unknown_defaults_cool);
    RUN_TEST(test_fan_round_trip);
    RUN_TEST(test_fan_5_maps_to_4);
    RUN_TEST(test_fan_unknown_defaults_1);
    RUN_TEST(test_vane_ud_round_trip);
    RUN_TEST(test_vane_ud_unknown_defaults_up);
    RUN_TEST(test_vane_lr_round_trip);
    RUN_TEST(test_vane_lr_6_is_wide_alias);
    RUN_TEST(test_vane_lr_unknown_defaults_left);
    return UNITY_END();
}
