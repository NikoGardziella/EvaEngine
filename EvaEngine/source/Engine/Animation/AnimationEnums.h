#pragma once
#include <cstdint>

namespace Engine {

    enum class Dir8 : uint8_t { E = 0, SE = 1, S = 2, SW = 3, W = 4, NW = 5, N = 6, NE = 7 };

    inline const char* ToString(Dir8 d) {
        switch (d) {
        case Dir8::E:  return "E";
        case Dir8::SE: return "SE";
        case Dir8::S:  return "S";
        case Dir8::SW: return "SW";
        case Dir8::W:  return "W";
        case Dir8::NW: return "NW";
        case Dir8::N:  return "N";
        case Dir8::NE: return "NE";
        }
        return "E";
    }

}
