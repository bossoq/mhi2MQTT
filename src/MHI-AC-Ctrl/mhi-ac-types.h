#pragma once
// Hardware-free MHI A/C enums, shared between the SPI driver and the
// application layer. Keep this header free of FreeRTOS/ESP-IDF includes
// so it can be compiled in native (host) unit tests.
#include <stdint.h>

namespace mhi_ac
{
    enum class ACPower
    {
        power_off = 0,
        power_on = 1
    };

    enum class ACMode
    {
        mode_auto = 0b00000000,
        mode_dry = 0b00000100,
        mode_cool = 0b00001000,
        mode_fan = 0b00001100,
        mode_heat = 0b00010000,
        mode_unknown = 0xff
    };

    enum class ACFan
    {
        speed_1 = 0,
        speed_2 = 1,
        speed_3 = 2,
        speed_4 = 6,
        speed_auto = 7,
        unknown = 0xff,
    };

    enum class ACVanesUD
    { // Vanes enum
        Up = 0,
        UpCenter = 1,
        CenterDown = 2,
        Down = 3,
        Swing = 4,
        SeeIRRemote = 255
    };

    enum class ACVanesLR
    {
        Left = 0,
        LeftCenter = 1,
        Center = 2,
        CenterRight = 3,
        Right = 4,
        Wide = 5,
        Spot = 6,
        Swing = 8
    };
} // namespace mhi_ac
