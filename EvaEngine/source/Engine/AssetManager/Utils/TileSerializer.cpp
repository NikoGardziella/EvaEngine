
#include "pch.h"

#include "TileSerializer.h"
#include <fstream>
#include <yaml-cpp/yaml.h>
#include <filesystem>
#include "Engine/AssetManager/AssetManager.h"
#include <Engine/Core/Log.h>
#include <Engine/Scene/Components/Render/TileComponent.h>


namespace Engine {

    // At startup
    void TileSerializer::LoadAllTiles(std::unordered_map<std::string, TileProperties>& tiles)
    {
        YAML::Node data;
        try
        {
            data = YAML::LoadFile(GetTilePropertiesPath().string());
        }
        catch (const YAML::BadFile& e)
        {
            EE_CORE_WARN("No existing tile properties file found: {}", e.what());
            return;
        }

        for (auto it = data.begin(); it != data.end(); ++it)
        {
            TileProperties props;
            props.name = it->second["Name"].as<std::string>();
            props.health = it->second["Health"].as<int>();
            props.material = static_cast<eTileMaterial>(it->second["Material"].as<int>());

            auto uv = it->second["UV"];
            props.uv = glm::vec4(uv[0].as<float>(), uv[1].as<float>(),
                uv[2].as<float>(), uv[3].as<float>());

            tiles[it->first.as<std::string>()] = props;
        }
    }

    void TileSerializer::SaveTileDefinitions(const TileDefinitionRegistry& defs)
    {
        EE_PROFILE_FUNCTION();

        YAML::Emitter out;
        out << YAML::BeginMap;
        out << YAML::Key << "TileDefinitions" << YAML::Value << YAML::BeginSeq;

       

        for (const auto& [typeId, def] : defs.GetDefinitions())
        {
            out << YAML::BeginMap;

            out << YAML::Key << "TypeId" << YAML::Value << def.TypeId;
            out << YAML::Key << "Name" << YAML::Value << def.Name;

     

            out << YAML::Key << "Category" << YAML::Value << ToString(def.Category);
            out << YAML::Key << "Direction" << YAML::Value << TileDirectionToString(def.Direction);
            out << YAML::Key << "Material" << YAML::Value << ToString(def.Material);

            out << YAML::Key << "BaseHealth" << YAML::Value << def.BaseHealth;
            out << YAML::Key << "IsDestructible" << YAML::Value << def.IsDestructible;
            out << YAML::Key << "IsSupportingRoof" << YAML::Value << def.IsSupportingRoof;
            out << YAML::Key << "IsRoof" << YAML::Value << def.IsRoof;

            out << YAML::EndMap;
        }

        out << YAML::EndSeq;
        out << YAML::EndMap;

        std::filesystem::path path = GetTileDefinitionsPath();

        std::ofstream fout(path);
        fout << out.c_str();

        EE_CORE_INFO("Saved TileDefinitions to {}", path.string());
    }

    bool TileSerializer::LoadTileDefinitions(TileDefinitionRegistry& defs)
    {
        EE_PROFILE_FUNCTION();

        std::filesystem::path path = TileSerializer::GetTileDefinitionsPath();

        if (!std::filesystem::exists(path))
        {
            EE_CORE_WARN("TileDefinitions file not found: {}", path.string());
            return false;
        }

        YAML::Node data = YAML::LoadFile(path.string());

        if (!data["TileDefinitions"])
        {
            EE_CORE_ERROR("Invalid TileDefinitions file!");
            return false;
        }

        
        defs.Clear(); // IMPORTANT: avoid duplicates

        for (const auto& defNode : data["TileDefinitions"])
        {
            TileDefinition def{};
            TileTypeKey key{};

            def.TypeId = defNode["TypeId"].as<uint16_t>();
            def.Name = defNode["Name"].as<std::string>();
            /*
            if (auto uvNode = defNode["UV"])
            {
                def.UV.x = uvNode[0].as<float>();
                def.UV.y = uvNode[1].as<float>();
                def.UV.z = uvNode[2].as<float>();
                def.UV.w = uvNode[3].as<float>();
            }
            */

            def.Category = CategoryFromString(defNode["Category"].as<std::string>());
            def.Direction = TileDirectionFromString(defNode["Direction"].as<std::string>());
            def.Material = MaterialFromString(defNode["Material"].as<std::string>());

            def.BaseHealth = defNode["BaseHealth"].as<uint16_t>();
            def.IsDestructible = defNode["IsDestructible"].as<bool>();
            def.IsSupportingRoof = defNode["IsSupportingRoof"].as<bool>();
            def.IsRoof = defNode["IsRoof"].as<bool>();

            key.name = def.Name;
            key.category = def.Category;
            key.direction = def.Direction;

            defs.Register(def, key);
        }

        EE_CORE_INFO("Loaded TileDefinitions from {}", path.string());
        return true;
    }

    // When saving
    void TileSerializer::Save(const std::unordered_map<std::string, TileProperties>& editedTiles)
    {
        // 1. Load existing data so we don't lose it
        std::unordered_map<std::string, TileProperties> allTiles;
        LoadAllTiles(allTiles);

        // 2. Merge edits into the existing map
        for (const auto& pair : editedTiles)
            allTiles[pair.first] = pair.second;

        // 3. Write the merged set
        YAML::Emitter emitter;
        emitter << YAML::BeginMap;
        for (const auto& pair : allTiles)
        {
            emitter << YAML::Key << pair.first;
            emitter << YAML::Value << YAML::BeginMap;
            emitter << YAML::Key << "Name" << YAML::Value << pair.second.name;
            emitter << YAML::Key << "Health" << YAML::Value << pair.second.health;
            emitter << YAML::Key << "Material" << YAML::Value << static_cast<int>(pair.second.material);
            emitter << YAML::Key << "UV" << YAML::Value << YAML::Flow
                << YAML::BeginSeq
                << pair.second.uv.x << pair.second.uv.y
                << pair.second.uv.z << pair.second.uv.w
                << YAML::EndSeq;
            emitter << YAML::EndMap;
        }
        emitter << YAML::EndMap;

        std::ofstream fout(GetTilePropertiesPath());
        if (!fout.is_open())
        {
            EE_CORE_WARN("Could not open file for saving tile properties");
            return;
        }
        fout << emitter.c_str();
    }

    /*
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
            emitter << YAML::Key << "UV" << YAML::Value << YAML::Flow
                << YAML::BeginSeq
                << pair.second.uv.x << pair.second.uv.y
                << pair.second.uv.z << pair.second.uv.w
                << YAML::EndSeq;
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
    */


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

                    if (node["UV"] && node["UV"].IsSequence() && node["UV"].size() == 4)
                    {
                        props.uv = glm::vec4(
                            node["UV"][0].as<float>(),
                            node["UV"][1].as<float>(),
                            node["UV"][2].as<float>(),
                            node["UV"][3].as<float>()
                        );
                    }

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

    std::filesystem::path TileSerializer::GetTileDefinitionsPath()
    {
        return AssetManager::GetAssetFolderPath() / "textures" / "tiles" / "data" / "TileDefinitions.yaml";
    }
} 