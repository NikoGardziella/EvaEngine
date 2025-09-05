#pragma once

#include <string>
#include <unordered_map>
#include <filesystem> // For std::filesystem::path
#include <fstream>    // For file operations
#include <cstdint>    



#include "Engine/Scene/Components/Render/TileComponent.h"

namespace Engine {

    struct PixelRect { int x = 0, y = 0, w = 0, h = 0; };

    struct TileProperties {
        uint32_t        health = 0;
        eTileMaterial   material = eTileMaterial::None;
        eTileCategory   category = eTileCategory::Default;
        std::string     name;
        glm::vec4       uv = glm::vec4(0);
        PixelRect       pixelRect{};

        //how many bottom rows of the sprite should collide (0 = none)
        uint16_t        collisionFootRowsPx{ 0 };
        uint32_t        pivotYOffsetPx = 0;        // rows of transparent padding at bottom
        uint32_t        pivotXCenterOffsetPx = 0;
        bool            pivotAuto = true;
    };

    class TileSerializer {
    public:
        static void LoadAllTiles(std::unordered_map<std::string, TileProperties>& tiles);
        static void Save(const std::unordered_map<std::string, TileProperties>& tiles);

        static void Load(std::unordered_map<std::string, TileProperties>& tiles);

        static std::filesystem::path GetTilePropertiesPath();
    };


}