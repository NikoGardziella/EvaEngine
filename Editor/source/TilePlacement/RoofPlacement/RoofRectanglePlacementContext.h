#pragma once
#include <cstdint>
#include <Engine/Scene/Scene.h>
#include "RoofDirectionalTypeSet.h"

namespace Engine {

    struct RoofRectanglePlacementContext
    {
        Scene* ActiveScene = nullptr;
        CompactTileMap* CompactMap = nullptr;

        RoofDirectionalTypeSet TypeSet{};

        uint64_t    GroupId = 0;
        uint8_t     Flags = 0;
        uint8_t     Aux = 0;
        int16_t    Floor= 0;

    };
}