#pragma once
#include <glm/ext/vector_float2.hpp>
#include <Engine/Platform/Vulkan/VulkanTexture.h>
#include "Engine/Core/Core.h"

namespace Engine {

    // for editor
    

    enum class eTileCategory
    {
		Undefined = 0,
        Buildings,
        Terrain,
        Roofs
    };


    struct TileInfo {
        glm::vec2 position; // local position within group
        glm::vec4 UV;
        std::string name;
        bool IsDestructible;
        bool IsRoof;
        eTileCategory Category;
		TileInfo(const glm::vec2& pos = glm::vec2(0.0f), const glm::vec4& uvCoords = glm::vec4(0.0f),
            const std::string& tileName = "", bool destructible = false, bool roof = false,
            eTileCategory category = eTileCategory::Undefined)
			: position(pos), UV(uvCoords), name(tileName),
            IsDestructible(destructible), IsRoof(roof), Category(category)
        {
		}
    };

    struct TileComponent
    {
        uint32_t TileID;
        Ref<VulkanTexture> Texture;
        std::vector<TileInfo> tiles;
    };

    inline const char* ToString(eTileCategory category)
    {
        switch (category)
        {
        case eTileCategory::Undefined: return "Undefined";
        case eTileCategory::Buildings: return "Buildings";
        case eTileCategory::Terrain:   return "Terrain";
        case eTileCategory::Roofs:     return "Roofs";
        default: return "Unknown";
        }
    }

    inline eTileCategory CategoryFromString(const std::string& str)
    {
        if (str == "Buildings") return eTileCategory::Buildings;
        if (str == "Terrain") return eTileCategory::Terrain;
        if (str == "Roofs") return eTileCategory::Roofs;
        return eTileCategory::Undefined;
    }
}

