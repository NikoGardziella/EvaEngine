#pragma once
#include <Engine/Platform/Vulkan/VulkanTexture.h>

#include "glm/glm.hpp"
#include <cstdint>

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
        Terrain,
        Default,
        Buildings,
        Pillars,
        Vehicles,
        dynamicObjects,
        Roofs,
        Doors,
        Windows,
    };

    enum class eTileDirection : uint32_t
    {
        North = 0,
        South,
        East,
        West,
        Center, // BUllet light=

        Unknown
    };

    struct TileTypeKey
    {
        std::string name;
        glm::vec4 uv{};
        eTileCategory category{};
        eTileDirection direction{}; 

        bool operator==(const TileTypeKey& other) const
        {
            return name == other.name &&
                uv == other.uv &&
                category == other.category &&
                direction == other.direction;
        }
    };


    struct TileTypeKeyHash
    {
        size_t operator()(const TileTypeKey& k) const
        {
            size_t h = std::hash<std::string>{}(k.name);

            auto hashCombine = [](size_t& seed, size_t value)
                {
                    seed ^= value + 0x9e3779b9 + (seed << 6) + (seed >> 2);
                };

            hashCombine(h, std::hash<float>{}(k.uv.x));
            hashCombine(h, std::hash<float>{}(k.uv.y));
            hashCombine(h, std::hash<float>{}(k.uv.z));
            hashCombine(h, std::hash<float>{}(k.uv.w));
            hashCombine(h, std::hash<int>{}(static_cast<int>(k.category)));
            hashCombine(h, std::hash<int>{}(static_cast<int>(k.direction)));

            return h;
        }
    };

    struct TileInfo {
        glm::vec2       position; // local position within group
        glm::vec4       UV;
        std::string     name;
        uint64_t        UID;
        bool            IsDestructible;
        bool            IsRoof; // mayeb this should be " IsPlacedOnRood" 
        bool            IsSupportingRoof;
        bool            IsSpawned = false;
        eTileCategory   Category;
        eTileMaterial   Material;
        uint32_t        TileHealth;
        uint32_t        Slot = UINT32_MAX;
        eTileDirection   TileDirection;
        glm::ivec2      opaqueMin = { 999, 999 };
        glm::ivec2      opaqueMax = { 999, 999 };


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

    inline const char* TileDirectionToString(eTileDirection direction)
    {
        switch (direction)
        {
        case eTileDirection::North:      return "North";
        case eTileDirection::South:      return "South";
        case eTileDirection::East:       return "East";
        case eTileDirection::West:       return "West";
        case eTileDirection::Center:     return "Center";
        case eTileDirection::Unknown:    return "Unknown";

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

    inline eTileDirection TileDirectionFromString(const std::string& str)
    {
        if (str == "North" || str == "N") return eTileDirection::North;
        if (str == "South" || str == "S") return eTileDirection::South;
        if (str == "East" || str == "E") return eTileDirection::East;
        if (str == "West" || str == "W") return eTileDirection::West;
        if (str == "Center") return eTileDirection::Center;
        if (str == "Unkown") return eTileDirection::Unknown;

        return eTileDirection::Unknown;
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

