#include "pch.h"
#include "PrefabSerializer.h"
#include <yaml-cpp/yaml.h>
#include "Engine/AssetManager/AssetManager.h"

namespace Engine {

    void PrefabSerializer::Serialize(const std::string& filepath, Entity rootEntity)
    {
        std::filesystem::path filePath(filepath);

        if (!std::filesystem::exists(filePath.parent_path()))
            std::filesystem::create_directories(filePath.parent_path());

        YAML::Emitter out;
        out << YAML::BeginMap;

        out << YAML::Key << "Prefab" << YAML::Value << "MyPrefab";
        out << YAML::Key << "Version" << YAML::Value << 1;

        if (rootEntity.HasComponent<TagComponent>())
            out << YAML::Key << "RootName" << YAML::Value << rootEntity.GetComponent<TagComponent>().Tag;
        else
            out << YAML::Key << "RootName" << YAML::Value << "Unnamed";

        glm::vec3 origin = { 0.0f, 0.0f, 0.0f };
        if (rootEntity.HasComponent<TransformComponent>())
            origin = rootEntity.GetComponent<TransformComponent>().Translation;

        out << YAML::Key << "Origin" << YAML::Value << YAML::Flow << YAML::BeginSeq
            << origin.x << origin.y << origin.z
            << YAML::EndSeq;

        out << YAML::Key << "Tiles" << YAML::Value << YAML::BeginSeq;
        SerializeTilesFromTileComponent(rootEntity, out);
        out << YAML::EndSeq;

        out << YAML::EndMap;

        std::ofstream fout(filepath);
        fout << out.c_str();
    }


    Entity PrefabSerializer::Deserialize(const std::string& filepath, const glm::ivec2& spawnCell)
    {
        if (!m_scene)
        {
            EE_CORE_ERROR("PrefabSerializer::Deserialize failed, scene is null");
            return {};
        }

        YAML::Node data = YAML::LoadFile(filepath);

        if (!data["Prefab"])
        {
            EE_CORE_ERROR("Invalid prefab file '{}': missing Prefab key", filepath);
            return {};
        }

        std::string rootName = "PrefabInstance";
        if (data["RootName"])
            rootName = data["RootName"].as<std::string>();

        Entity newEntity = m_scene->CreateEntity(rootName);
        if (!newEntity)
        {
            EE_CORE_ERROR("Failed to create prefab root entity");
            return {};
        }

        // Place the prefab root in the world
        if (newEntity.HasComponent<TransformComponent>())
        {
            TransformComponent& transform = newEntity.GetComponent<TransformComponent>();
            transform.Translation = glm::vec3((float)spawnCell.x, (float)spawnCell.y, 0.0f);
        }
        else
        {
            newEntity.AddComponent<TransformComponent>();
        }

        // Rebuild the TileComponent from prefab data
        if (data["Tiles"])
        {
            DeserializeTilesToTileComponent(newEntity, data["Tiles"], glm::ivec2(0));
        }
        else
        {
            EE_CORE_WARN("Prefab '{}' has no Tiles section", filepath);
        }

        EE_CORE_INFO("Prefab loaded: {} at ({}, {})", filepath, spawnCell.x, spawnCell.y);
        return newEntity;
    }

    inline void PrefabSerializer::DeserializeCompactTilesPrefab(
        Scene& scene,
        const YAML::Node& compactTilesNode,
        const glm::ivec2& spawnCell,
        uint64_t newGroupId)
    {
        CompactTileMap& compactMap = scene.GetCompactTileMap();

        for (const auto& tileNode : compactTilesNode)
        {
            glm::ivec2 localCell{};

            if (auto cellNode = tileNode["Cell"])
            {
                localCell.x = cellNode[0].as<int>();
                localCell.y = cellNode[1].as<int>();
            }
            else
            {
                continue;
            }

            const glm::ivec2 worldCell = spawnCell + localCell;

            CompactTile tile{};
            tile.TypeId = tileNode["TypeId"].as<uint16_t>();
            tile.Flags = CompactTileFlags::None;
            tile.Aux = static_cast<uint8_t>(tileNode["Aux"].as<int>());
            tile.GroupId = newGroupId;

            if (tile.IsEmpty())
                continue;

            // Do not duplicate same TypeId in the same cell
            if (compactMap.HasTileType(worldCell, tile.TypeId))
            {
                EE_CORE_WARN(
                    "DeserializeCompactTilesPrefab: tile type {} already exists at cell ({}, {})",
                    tile.TypeId, worldCell.x, worldCell.y);
                continue;
            }

            compactMap.AddTile(worldCell, tile);
            compactMap.RegisterCellForGroup(tile.GroupId, worldCell);
            compactMap.MarkChunkDirtyForCell(worldCell);
        }
    }


    inline glm::ivec2 WorldToCell(const glm::vec2& world)
    {
        return glm::ivec2(
            static_cast<int>(std::round(world.x)),
            static_cast<int>(std::round(world.y))
        );
    }

    inline void PrefabSerializer::SerializeTilesFromTileComponent(Entity rootEntity, YAML::Emitter& out)
    {
        if (!rootEntity.HasComponent<TileComponent>())
        {
            EE_CORE_WARN("Selected entity has no TileComponent, no tiles saved to prefab.");
            return;
        }

        TileComponent& tileComp = rootEntity.GetComponent<TileComponent>();

        for (const auto& tile : tileComp.tiles)
        {
            out << YAML::BeginMap;

            out << YAML::Key << "Position" << YAML::Value << YAML::Flow
                << std::vector<float>{ tile.position.x, tile.position.y };
            /*
            out << YAML::Key << "UV" << YAML::Value << YAML::Flow
                << std::vector<float>{ tile.UV.x, tile.UV.y, tile.UV.z, tile.UV.w };
            */

            out << YAML::Key << "Name" << YAML::Value << tile.name;
            out << YAML::Key << "UID" << YAML::Value << tile.UID;

            out << YAML::Key << "IsDestructible" << YAML::Value << tile.IsDestructible;
            out << YAML::Key << "IsRoof" << YAML::Value << tile.IsRoof;
            out << YAML::Key << "IsSupportingRoof" << YAML::Value << tile.IsSupportingRoof;

            out << YAML::Key << "Category" << YAML::Value << static_cast<int>(tile.Category);
            out << YAML::Key << "Material" << YAML::Value << static_cast<int>(tile.Material);
            out << YAML::Key << "TileHealth" << YAML::Value << tile.TileHealth;
            out << YAML::Key << "Slot" << YAML::Value << tile.Slot;
            out << YAML::Key << "TileDirection" << YAML::Value << static_cast<int>(tile.TileDirection);

            out << YAML::Key << "OpaqueMin" << YAML::Value << YAML::Flow
                << std::vector<int>{ tile.opaqueMin.x, tile.opaqueMin.y };

            out << YAML::Key << "OpaqueMax" << YAML::Value << YAML::Flow
                << std::vector<int>{ tile.opaqueMax.x, tile.opaqueMax.y };

            out << YAML::EndMap;
        }
    }

    inline void PrefabSerializer::SerializeCompactTiles(Ref<Scene> scene, Entity rootEntity, YAML::Emitter& out)
    {
        if (!rootEntity)
            return;

        const CompactTileMap& compactMap = scene->GetCompactTileMap();

        glm::vec3 originWorld = { 0.0f, 0.0f, 0.0f };
        if (rootEntity.HasComponent<TransformComponent>())
            originWorld = rootEntity.GetComponent<TransformComponent>().Translation;

        glm::ivec2 originCell = WorldToCell({ originWorld.x, originWorld.y });

        uint64_t targetGroupId = 0;
        if (rootEntity.HasComponent<IDComponent>())
            targetGroupId = rootEntity.GetComponent<IDComponent>().ID;
        else
            return;

        for (const auto& [chunkCoord, chunk] : compactMap.GetChunks())
        {
            const glm::ivec2 chunkOriginCell = ChunkCoordToWorldOrigin(chunkCoord);

            for (int y = 0; y < TILE_CHUNK_H; ++y)
            {
                for (int x = 0; x < TILE_CHUNK_W; ++x)
                {
                    const glm::ivec2 worldCell = chunkOriginCell + glm::ivec2(x, y);
                    const std::vector<CompactTile>* tiles = compactMap.GetTiles(worldCell);
                    if (!tiles || tiles->empty())
                        continue;

                    const glm::ivec2 localCell = worldCell - originCell;

                    for (const CompactTile& tile : *tiles)
                    {
                        if (tile.IsEmpty() || tile.GroupId != targetGroupId)
                            continue;

                        out << YAML::BeginMap;
                        out << YAML::Key << "Cell" << YAML::Value << YAML::Flow
                            << std::vector<int>{ localCell.x, localCell.y };
                        out << YAML::Key << "TypeId" << YAML::Value << tile.TypeId;
                        out << YAML::Key << "Flags" << YAML::Value << (int)tile.Flags;
                        out << YAML::Key << "Aux" << YAML::Value << (int)tile.Aux;
                        out << YAML::EndMap;
                    }
                }
            }
        }
    }
    inline void PrefabSerializer::DeserializeTilesToTileComponent(
        Entity rootEntity,
        const YAML::Node& tilesNode,
        const glm::vec2& worldOffset)
    {
        if (!rootEntity.HasComponent<TileComponent>())
            rootEntity.AddComponent<TileComponent>();

        TileComponent& tileComp = rootEntity.GetComponent<TileComponent>();
        tileComp.tiles.clear();

        for (const auto& tileNode : tilesNode)
        {
            TileInfo tile{};

            if (auto posNode = tileNode["Position"])
            {
                tile.position.x = posNode[0].as<float>();
                tile.position.y = posNode[1].as<float>();
            }
          
            

            if (tileNode["Name"])
                tile.name = tileNode["Name"].as<std::string>();


            auto& props = AssetManager::GetTileProperties(tile.name);
            tile.UV = props.uv;


            if (tileNode["UID"])
                tile.UID = tileNode["UID"].as<uint64_t>();

            if (tileNode["IsDestructible"])
                tile.IsDestructible = tileNode["IsDestructible"].as<bool>();

            if (tileNode["IsRoof"])
                tile.IsRoof = tileNode["IsRoof"].as<bool>();

            if (tileNode["IsSupportingRoof"])
                tile.IsSupportingRoof = tileNode["IsSupportingRoof"].as<bool>();

            if (tileNode["Category"])
                tile.Category = static_cast<eTileCategory>(tileNode["Category"].as<int>());

            if (tileNode["Material"])
                tile.Material = static_cast<eTileMaterial>(tileNode["Material"].as<int>());

            if (tileNode["TileHealth"])
                tile.TileHealth = tileNode["TileHealth"].as<uint32_t>();

            if (tileNode["Slot"])
                tile.Slot = tileNode["Slot"].as<uint32_t>();

            if (tileNode["TileDirection"])
                tile.TileDirection = static_cast<eTileDirection>(tileNode["TileDirection"].as<int>());

            if (auto minNode = tileNode["OpaqueMin"])
            {
                tile.opaqueMin.x = minNode[0].as<int>();
                tile.opaqueMin.y = minNode[1].as<int>();
            }

            if (auto maxNode = tileNode["OpaqueMax"])
            {
                tile.opaqueMax.x = maxNode[0].as<int>();
                tile.opaqueMax.y = maxNode[1].as<int>();
            }

            
            tile.position += worldOffset;

            tileComp.tiles.push_back(tile);
        }
    }


}