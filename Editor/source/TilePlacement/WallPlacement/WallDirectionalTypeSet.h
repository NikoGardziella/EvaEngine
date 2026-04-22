#pragma once
#include <cstdint>

namespace Engine {

    struct WallDirectionalTypeSet
    {
        uint16_t North = 0;
        uint16_t South = 0;
        uint16_t East = 0;
        uint16_t West = 0;

        bool IsValid() const
        {
            return North != 0 && South != 0 && East != 0 && West != 0;
        }
    };
}