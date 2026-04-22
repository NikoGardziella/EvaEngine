#pragma once
#include <Engine/Scene/Scene.h>

namespace Engine {

    struct TerrainRectanglePlacementContext
    {
        Scene* ActiveScene = nullptr;
        CompactTileMap* CompactMap = nullptr;

        uint16_t TypeId = 0;
        uint64_t GroupId = 0;

        uint8_t Flags = Engine::CompactTileFlags::None;
        uint8_t Aux = 0;
    };
}