#pragma once
#include <yaml-cpp/emitter.h>

namespace Engine {

    class PrefabSerializer
    {
    public:
        PrefabSerializer(const Ref<Scene>& scene)
            : m_scene(scene) {
        }

        void Serialize(const std::string& filepath, Entity rootEntity);

        Entity Deserialize(const std::string& filepath, const glm::ivec2& spawnCell);

        void DeserializeCompactTilesPrefab(Scene& scene, const YAML::Node& compactTilesNode, const glm::ivec2& spawnCell, uint64_t newGroupId);


        void SerializeTilesFromTileComponent(Entity rootEntity, YAML::Emitter& out);

        void SerializeCompactTiles(Ref<Scene> scene, Entity rootEntity, YAML::Emitter& out);


        void DeserializeTilesToTileComponent(Entity rootEntity, const YAML::Node& tilesNode, const glm::vec2& worldOffset);

        uint16_t GetOrCreateDefinitionForRuntimeTile(Scene& scene, const TileInfo& tile);


    private:
        Ref<Scene> m_scene;
    };
}


