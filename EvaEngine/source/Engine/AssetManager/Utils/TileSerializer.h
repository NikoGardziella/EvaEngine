#pragma once

#include <string>
#include <unordered_map>
#include <filesystem> // For std::filesystem::path
#include <fstream>    // For file operations
#include <cstdint>    



#include "Engine/Scene/Components/Render/TileComponent.h"

namespace Engine {



    struct TileProperties {
        uint32_t health = 0;
        eTileMaterial material = eTileMaterial::None;
        std::string name;
    };

    class TileSerializer {
    public:
        static void Save(const std::unordered_map<std::string, TileProperties>& tiles);

        static void Load(std::unordered_map<std::string, TileProperties>& tiles);

        static std::filesystem::path GetTilePropertiesPath();
    };


}