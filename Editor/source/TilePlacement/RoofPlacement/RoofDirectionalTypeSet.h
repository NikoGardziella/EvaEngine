#pragma once
#include <cstdint>

namespace Engine {

    struct RoofDirectionalTypeSet
    {
        uint16_t CornerNorth = 0;
        uint16_t CornerSouth = 0;
        uint16_t CornerEast = 0;
        uint16_t CornerWest = 0;

        uint16_t EdgeNorth = 0;
        uint16_t EdgeSouth = 0;
        uint16_t EdgeEast = 0;
        uint16_t EdgeWest = 0;

        uint16_t Fill = 0;

        bool IsValid() const
        {
            return CornerNorth != 0 && CornerSouth != 0 &&
                CornerEast != 0 && CornerWest != 0 &&
                EdgeNorth != 0 && EdgeSouth != 0 &&
                EdgeEast != 0 && EdgeWest != 0 &&
                Fill != 0;
        }
    };
}