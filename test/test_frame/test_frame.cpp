// Native unit tests for the MHI frame codec (signature/checksum/temperature).
// Run: pio test -e native
#include <unity.h>
#include "../../src/MHI-AC-Ctrl/mhi-frame.cpp"

using namespace mhi_ac;
using namespace mhi_ac::internal;

using Frame = std::array<uint8_t, MHI_FRAME_LEN_LONG>;

void setUp(void) {}
void tearDown(void) {}

// Builds a frame with a valid signature and correct checksums
static Frame make_valid_frame()
{
    Frame f{};
    f[SB0] = 0x6c;
    f[SB1] = 0x80;
    f[SB2] = 0x04;
    f[DB0] = 0x11;
    f[DB2] = 0xB2; // setpoint 25.0C with set bit
    f[DB3] = 0x9D; // room temp (157-61)/4 = 24.0C
    frame::apply_tx_checksums(f);
    return f;
}

// ---- signature ----

static void test_signature_valid(void)
{
    TEST_ASSERT_TRUE(frame::validate_signature(0x6c, 0x80, 0x04));
    TEST_ASSERT_TRUE(frame::validate_signature(0x6d, 0x80, 0x04)); // bit 0 masked
}

static void test_signature_invalid(void)
{
    TEST_ASSERT_FALSE(frame::validate_signature(0x00, 0x80, 0x04));
    TEST_ASSERT_FALSE(frame::validate_signature(0x6c, 0x81, 0x04));
    TEST_ASSERT_FALSE(frame::validate_signature(0x6c, 0x80, 0x05));
    TEST_ASSERT_FALSE(frame::validate_signature(0x6e, 0x80, 0x04)); // bit 1 not masked
}

// ---- frame validation ----

static void test_valid_short_frame(void)
{
    Frame f = make_valid_frame();
    TEST_ASSERT_EQUAL_INT(0, frame::validate_frame(f, MHI_FRAME_LEN_SHORT));
}

static void test_valid_long_frame(void)
{
    Frame f = make_valid_frame();
    TEST_ASSERT_EQUAL_INT(0, frame::validate_frame(f, MHI_FRAME_LEN_LONG));
}

static void test_bad_signature_rejected(void)
{
    Frame f = make_valid_frame();
    f[SB1] = 0x00;
    TEST_ASSERT_EQUAL_INT(frame::ERR_SIGNATURE, frame::validate_frame(f, MHI_FRAME_LEN_SHORT));
}

static void test_corrupt_data_byte_fails_short_checksum(void)
{
    Frame f = make_valid_frame();
    f[DB9] ^= 0x01; // flip one bit after checksums were applied
    TEST_ASSERT_EQUAL_INT(frame::ERR_CHECKSUM_SHORT, frame::validate_frame(f, MHI_FRAME_LEN_SHORT));
}

static void test_corrupt_long_section_fails_long_checksum(void)
{
    Frame f = make_valid_frame();
    f[DB20] ^= 0x01; // corrupt a byte only covered by the long checksum
    TEST_ASSERT_EQUAL_INT(0, frame::validate_frame(f, MHI_FRAME_LEN_SHORT));
    TEST_ASSERT_EQUAL_INT(frame::ERR_CHECKSUM_LONG, frame::validate_frame(f, MHI_FRAME_LEN_LONG));
}

static void test_tx_checksum_round_trip(void)
{
    Frame f{};
    f[SB0] = 0x6c;
    f[SB1] = 0x80;
    f[SB2] = 0x04;
    for (int i = DB0; i <= DB14; i++)
        f[i] = (uint8_t)(i * 7); // arbitrary payload
    frame::apply_tx_checksums(f);
    TEST_ASSERT_EQUAL_INT(0, frame::validate_frame(f, MHI_FRAME_LEN_LONG));
}

// ---- temperature codec ----

static void test_target_temp_round_trip(void)
{
    for (float t = 17.0f; t <= 30.0f; t += 0.5f)
    {
        uint8_t db2 = frame::target_temp_encode(t);
        TEST_ASSERT_EQUAL_FLOAT(t, frame::target_temp_decode(db2));
    }
}

static void test_target_temp_set_bit(void)
{
    TEST_ASSERT_TRUE(frame::target_temp_encode(25.0f) & 0x80);
}

static void test_target_temp_clamps(void)
{
    // above 30C clamps to 30C; below 0 clamps to 0
    TEST_ASSERT_EQUAL_FLOAT(30.0f, frame::target_temp_decode(frame::target_temp_encode(99.0f)));
    TEST_ASSERT_EQUAL_FLOAT(0.0f, frame::target_temp_decode(frame::target_temp_encode(-5.0f)));
}

static void test_target_temp_rounds_to_half_degree(void)
{
    TEST_ASSERT_EQUAL_FLOAT(25.5f, frame::target_temp_decode(frame::target_temp_encode(25.6f)));
    TEST_ASSERT_EQUAL_FLOAT(25.0f, frame::target_temp_decode(frame::target_temp_encode(25.2f)));
}

static void test_room_temp_decode(void)
{
    TEST_ASSERT_EQUAL_FLOAT(24.0f, frame::room_temp_decode(0x9D)); // (157-61)/4
    TEST_ASSERT_EQUAL_FLOAT(0.0f, frame::room_temp_decode(61));
    TEST_ASSERT_EQUAL_FLOAT(-15.25f, frame::room_temp_decode(0));
}

static void test_room_temp_encode_round_trip(void)
{
    for (float t = 10.0f; t <= 35.0f; t += 0.25f)
    {
        uint8_t db3 = frame::room_temp_encode(t);
        TEST_ASSERT_EQUAL_FLOAT(t, frame::room_temp_decode(db3));
    }
}

int main(int argc, char **argv)
{
    UNITY_BEGIN();
    RUN_TEST(test_signature_valid);
    RUN_TEST(test_signature_invalid);
    RUN_TEST(test_valid_short_frame);
    RUN_TEST(test_valid_long_frame);
    RUN_TEST(test_bad_signature_rejected);
    RUN_TEST(test_corrupt_data_byte_fails_short_checksum);
    RUN_TEST(test_corrupt_long_section_fails_long_checksum);
    RUN_TEST(test_tx_checksum_round_trip);
    RUN_TEST(test_target_temp_round_trip);
    RUN_TEST(test_target_temp_set_bit);
    RUN_TEST(test_target_temp_clamps);
    RUN_TEST(test_target_temp_rounds_to_half_degree);
    RUN_TEST(test_room_temp_decode);
    RUN_TEST(test_room_temp_encode_round_trip);
    return UNITY_END();
}
