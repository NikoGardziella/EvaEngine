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
        Vehicles,
        dynamicObjects
    };


    struct TileInfo {
        glm::vec2   position; // local position within group
        glm::vec4   UV;
        std::string name;
        uint64_t    UID;
        bool        IsDestructible;
        bool        IsRoof;
        eTileCategory Category;
        eTileMaterial Material;
        uint32_t    TileHealth;
        TileInfo(const glm::vec2& pos = glm::vec2(0.0f), const glm::vec4& uvCoords = glm::vec4(0.0f),
            const std::string& tileName = "", bool destructible = false, bool roof = false,
            eTileCategory category = eTileCategory::Undefined, eTileMaterial material = eTileMaterial::Undefined,
            uint32_t tileHealth = 1, uint64_t  uid = 0
        )
			: position(pos), UV(uvCoords), name(tileName),
            IsDestructible(destructible), IsRoof(roof), Category(category),
            Material(material), TileHealth(tileHealth), UID(uid)
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
        case eTileCategory::dynamicObjects:  return "dynamicObjects";

        default: return "Unknown";
        }
    }

    

    inline const char* ToString(eTileMaterial material)
    {
        switch (material)
        {
        case eTileMaterial::Undefined: return "Undefined";
        case eTileMaterial::Default:   return "Default";
        case eTileMaterial::None:      return "None";
        case eTileMaterial::Wood:      return "Wood";
        case eTileMaterial::Concrete:  return "Concrete";
        case eTileMaterial::Steel:     return "Steel";
        case eTileMaterial::Stone:     return "Stone";
        case eTileMaterial::Plastic:   return "Plastic";
        case eTileMaterial::Metal:     return "Metal";
        case eTileMaterial::Glass:     return "Glass";
        default:                      return "Unknown";
        }
    }

    inline eTileMaterial MaterialFromString(const std::string& str)
    {
        if (str == "None")     return eTileMaterial::None;
        if (str == "Wood")     return eTileMaterial::Wood;
        if (str == "Concrete") return eTileMaterial::Concrete;
        if (str == "Steel") return eTileMaterial::Steel;
        if (str == "Metal")    return eTileMaterial::Metal;
        if (str == "Glass")    return eTileMaterial::Glass;
        return eTileMaterial::None;
    }


    inline eTileCategory CategoryFromString(const std::string& str)
    {
        if (str == "Buildings") return eTileCategory::Buildings;
        if (str == "Terrain") return eTileCategory::Terrain;
        if (str == "Roofs") return eTileCategory::Roofs;
        if (str == "Vehicles") return eTileCategory::Vehicles;
        if (str == "dynamicObjects") return eTileCategory::dynamicObjects;
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
        case eTileMaterial::Steel:    return "Steel";
        case eTileMaterial::Concrete: return "Concrete";
        case eTileMaterial::None:     return "None";
        case eTileMaterial::Undefined: return "Undefined";
        case eTileMaterial::Default:   return "Default";
        default:                      return "Unknown";
        }
    }




   


}

