#pragma once
#include <Engine/Platform/Vulkan/VulkanTexture.h>

#include "glm/glm.hpp"

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

    enum class TileDirection : uint32_t
    {
        North = 0,
        South,
        East,
        West,
        Center, // BUllet light=

        Unknown
    };

    struct TileInfo {
        glm::vec2       position; // local position within group
        glm::vec4       UV;
        std::string     name;
        uint64_t        UID;
        bool            IsDestructible;
        bool            IsRoof;
        eTileCategory   Category;
        eTileMaterial   Material;
        uint32_t        TileHealth;
        uint32_t        Slot = UINT32_MAX;
        TileDirection   TileDirection;
        glm::ivec2      opaqueMin;
        glm::ivec2      opaqueMax;


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

    inline const char* TileDirectionToString(TileDirection direction)
    {
        switch (direction)
        {
        case TileDirection::North:      return "North";
        case TileDirection::South:      return "South";
        case TileDirection::East:       return "East";
        case TileDirection::West:       return "West";
        case TileDirection::Center:     return "Center";
        case TileDirection::Unknown:    return "Unknown";

        default: return "Invalid";
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

    inline TileDirection TileDirectionFromString(const std::string& str)
    {
        if (str == "North" || str == "N") return TileDirection::North;
        if (str == "South" || str == "S") return TileDirection::South;
        if (str == "East" || str == "E") return TileDirection::East;
        if (str == "West" || str == "W") return TileDirection::West;
        if (str == "Center") return TileDirection::Center;
        if (str == "Unkown") return TileDirection::Unknown;

        return TileDirection::Unknown;
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

