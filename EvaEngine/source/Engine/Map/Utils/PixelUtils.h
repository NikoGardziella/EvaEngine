#pragma once
#include <cstdint>

namespace Engine {

    class PixelUtils
    {

    public:
        uint16_t PackPixel(uint8_t r, uint8_t g, uint8_t b, uint8_t health)
        {
            r = (r >> 4) & 0xF;
            g = (g >> 4) & 0xF;
            b = (b >> 4) & 0xF;

            return (r) | (g << 4) | (b << 8) | ((health & 0xF) << 12);
        }

        void UnpackPixel(uint16_t packed, uint8_t& r, uint8_t& g, uint8_t& b, uint8_t& health)
        {
            r = ((packed >> 0) & 0xF) << 4;
            g = ((packed >> 4) & 0xF) << 4;
            b = ((packed >> 8) & 0xF) << 4;
            health = (packed >> 12) & 0xF;
        }
    };


}