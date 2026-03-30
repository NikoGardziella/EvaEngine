#pragma once

#include <string>
#include <glm/glm.hpp>


namespace Engine
{
    struct TileDefinition
    {
        uint16_t TypeId = 0;

        std::string Name;
        glm::vec4 UV{};

        eTileCategory Category = eTileCategory::Terrain;
        eTileDirection Direction = eTileDirection::East;
        eTileMaterial Material = eTileMaterial::None;

        uint16_t BaseHealth = 1;

        bool IsDestructible = false;
        bool IsSupportingRoof = false;
        bool IsRoof = false;
    };
}