
#include "pch.h"

#include "TileSerializer.h"
#include <fstream>
#include <yaml-cpp/yaml.h>
#include <filesystem>
#include "Engine/AssetManager/AssetManager.h"
#include <Engine/Core/Log.h>


namespace Engine {

    void TileSerializer::Save(const std::unordered_map<std::string, TileProperties>& tiles) {
        YAML::Emitter emitter;
        emitter << YAML::BeginMap;
        for (const auto& pair : tiles)
        {
            emitter << YAML::Key << pair.first;
            emitter << YAML::Value << YAML::BeginMap;
            emitter << YAML::Key << "Name" << YAML::Value << pair.second.name;
            emitter << YAML::Key << "Health" << YAML::Value << pair.second.health;
            emitter << YAML::Key << "Material" << YAML::Value << static_cast<int>(pair.second.material);
            emitter << YAML::EndMap;

        }
        emitter << YAML::EndMap;

        std::ofstream fout(GetTilePropertiesPath());
        if (fout.is_open())
        {
            fout << emitter.c_str();
            fout.close();

        }
        else
        {
            EE_CORE_WARN("Could not open file for saving tile properties");
        }
    }

    void TileSerializer::Load(std::unordered_map<std::string, TileProperties>& tiles)
    {
        tiles.clear(); // Clear existing tiles before loading

        std::filesystem::path path = GetTilePropertiesPath();
        if (!std::filesystem::exists(path))
        {
            EE_CORE_WARN("missing file {}");
            return;
        }

        try {
            YAML::Node config = YAML::LoadFile(path.string());
            if (config.IsMap())
            {
                for (YAML::const_iterator it = config.begin(); it != config.end(); ++it)
                {
                    std::string tileName = it->first.as<std::string>();
                    const YAML::Node& node = it->second;

                    TileProperties props;
                    props.name = node["Name"] ? node["Name"].as<std::string>() : "UnnamedTile";
                    props.health = node["Health"] ? node["Health"].as<uint32_t>() : 0;
                    props.material = node["Material"]
                        ? static_cast<eTileMaterial>(node["Material"].as<int>())
                        : eTileMaterial::None;

                    tiles[tileName] = props;
                }
            }
        }
        catch (const YAML::BadFile& e)
        {
           EE_CORE_WARN("Bad file {}", e.msg);
        }
        catch (const YAML::Exception& e)
        {
            EE_CORE_WARN(" Exception {}", e.msg);
        }
    }


    std::filesystem::path TileSerializer::GetTilePropertiesPath()
    {
        return AssetManager::GetAssetFolderPath() / "textures" / "tiles" / "data" / "TileProperties.yaml";
    }

} 