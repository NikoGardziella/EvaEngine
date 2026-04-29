#pragma once
#include <Engine/Scene/Scene.h>
#include "WallDirectionalTypeSet.h"

namespace Engine {

    struct WallRectanglePlacementContext
    {
        Scene* ActiveScene = nullptr;
        CompactTileMap* CompactMap = nullptr;

        WallDirectionalTypeSet DirectionSet{};

        uint64_t    GroupId = 0;
        uint8_t     WallFlags = 0;
        uint8_t     WallAux = 0;
        int16_t    floor;
    };
}