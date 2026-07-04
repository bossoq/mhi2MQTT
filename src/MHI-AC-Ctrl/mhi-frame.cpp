#include <math.h>
#include "mhi-frame.h"

#ifndef MIN
#define MIN(a, b) ((a) < (b) ? (a) : (b))
#endif
#ifndef MAX
#define MAX(a, b) ((a) > (b) ? (a) : (b))
#endif
#define FRAME_CLAMP(x, lower, upper) (MIN(upper, MAX(x, lower)))

namespace mhi_ac
{
    namespace frame
    {
        using namespace mhi_ac::internal;

        bool validate_signature(uint8_t sb0, uint8_t sb1, uint8_t sb2)
        {
            return (sb0 & 0xfe) == 0x6c && sb1 == 0x80 && sb2 == 0x04;
        }

        static int validate_frame_short(const std::array<uint8_t, MHI_FRAME_LEN_LONG> &mosi_frame, uint16_t rx_checksum)
        {
            if (!validate_signature(mosi_frame[SB0], mosi_frame[SB1], mosi_frame[SB2]))
            {
                return ERR_SIGNATURE;
            }
            else if ((mosi_frame[CBH] != (rx_checksum >> 8 & 0xff)) | (mosi_frame[CBL] != (rx_checksum & 0xff)))
            {
                return ERR_CHECKSUM_SHORT;
            }
            return 0;
        }

        static int validate_frame_long(const std::array<uint8_t, MHI_FRAME_LEN_LONG> &mosi_frame, uint8_t rx_checksum)
        {
            if (mosi_frame[CBL2] != rx_checksum)
            {
                return ERR_CHECKSUM_LONG;
            }
            return 0;
        }

        int validate_frame(const std::array<uint8_t, MHI_FRAME_LEN_LONG> &mosi_frame, uint8_t frame_len)
        {
            int err = 0;
            uint16_t rx_checksum = 0;
            // Frame len has been validated before to only be either MHI_FRAME_LEN_LONG or MHI_FRAME_LEN_SHORT
            for (uint8_t i = 0; i < frame_len; i++)
            {
                switch (i)
                {
                case CBH:
                    // validate checksum short
                    err = validate_frame_short(mosi_frame, rx_checksum);
                    if (err)
                    {
                        return err;
                    }
                    rx_checksum += mosi_frame[CBH];
                    rx_checksum += mosi_frame[CBL];
                    // skip over CBL
                    i++;
                    break;
                case CBL2:
                    err = validate_frame_long(mosi_frame, rx_checksum);
                    if (err)
                    {
                        return err;
                    }
                    break;
                default:
                    rx_checksum += mosi_frame[i];
                }
            }
            return err;
        }

        void apply_tx_checksums(std::array<uint8_t, MHI_FRAME_LEN_LONG> &frame)
        {
            // calculate checksum for the short frame
            uint16_t tx_checksum = 0;
            for (uint8_t byte_cnt = 0; byte_cnt < CBH; byte_cnt++)
            {
                tx_checksum += frame[byte_cnt];
            }
            frame[CBH] = tx_checksum >> 8;
            frame[CBL] = tx_checksum;

            // Continue calculating for the long frame
            for (uint8_t byte_cnt = CBH; byte_cnt < CBL2; byte_cnt++)
            {
                tx_checksum += frame[byte_cnt];
            }
            frame[CBL2] = tx_checksum;
        }

        uint8_t target_temp_encode(float celsius)
        {
            uint8_t tsetpoint = (uint8_t)roundf(FRAME_CLAMP(celsius * 2, 0.f * 2, 30.f * 2));
            return tsetpoint | (1 << 7); // set bit for temp
        }

        float target_temp_decode(uint8_t db2)
        {
            float temp = db2 & ~(1 << 7);
            return temp / 2.0f;
        }

        float room_temp_decode(uint8_t db3)
        {
            return ((int)db3 - 61) / 4.0f;
        }

        uint8_t room_temp_encode(float celsius)
        {
            return (uint8_t)roundf(FRAME_CLAMP(celsius * 4 + 61, 0, 0xfe));
        }
    } // namespace frame
} // namespace mhi_ac
