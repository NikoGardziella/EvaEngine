#pragma once
#include <glm/ext/vector_float2.hpp>
#include <Engine/Platform/Vulkan/VulkanTexture.h>
#include "Engine/Core/Core.h"


namespace Engine {

    // for editor
    enum class eTileMaterial
    {
        Undefined = 0,
        Default = 1,
        None, 
        Wood,
        Concrete,
        Steel,
        Stone,
        Plastic,
        Metal,
        Glass,
        COUNT
    };

    enum class eTileCategory
    {
		Undefined = 0,
        Default = 1,
        Buildings,
        Terrain,
        Roofs,
        Vehicles
    };


    struct TileInfo {
        glm::vec2 position; // local position within group
        glm::vec4 UV;
        std::string name;
        bool IsDestructible;
        bool IsRoof;
        eTileCategory Category;
        eTileMaterial Material;
        uint32_t TileHealth;
        TileInfo(const glm::vec2& pos = glm::vec2(0.0f), const glm::vec4& uvCoords = glm::vec4(0.0f),
            const std::string& tileName = "", bool destructible = false, bool roof = false,
            eTileCategory category = eTileCategory::Undefined, eTileMaterial material = eTileMaterial::Undefined,
            uint32_t tileHealth = 1
        )
			: position(pos), UV(uvCoords), name(tileName),
            IsDestructible(destructible), IsRoof(roof), Category(category),
            Material(material), TileHealth(tileHealth)
        {
		}
    };

    struct TileComponent
    {
        uint32_t TileID;
        Ref<VulkanTexture> Texture;
        std::vector<TileInfo> tiles;
    };

    // for editor
    

    inline const char* ToString(eTileCategory category)
    {
        switch (category)
        {
        case eTileCategory::Undefined: return "Undefined";
        case eTileCategory::Buildings: return "Buildings";
        case eTileCategory::Terrain:   return "Terrain";
        case eTileCategory::Roofs:     return "Roofs";
        case eTileCategory::Vehicles:  return "Vehicles";

        default: return "Unknown";
        }
    }

    inline eTileCategory CategoryFromString(const std::string& str)
    {
        if (str == "Buildings") return eTileCategory::Buildings;
        if (str == "Terrain") return eTileCategory::Terrain;
        if (str == "Roofs") return eTileCategory::Roofs;
        if (str == "Vehicles") return eTileCategory::Vehicles;
        return eTileCategory::Undefined;
    }

    inline const char* GetTileMaterialName(eTileMaterial material)
    {
        switch (material)
        {
        case eTileMaterial::Wood:     return "Wood";
        case eTileMaterial::Stone:    return "Stone";
        case eTileMaterial::Metal:    return "Metal";
        case eTileMaterial::Glass:    return "Glass";
        case eTileMaterial::Plastic:  return "Plastic";
        case eTileMaterial::Concrete: return "Concrete";
        case eTileMaterial::None:     return "None";
        default:                      return "Unknown";
        }
    }




   


}

