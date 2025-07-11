#include "EditorUtils.h"
#include <Engine/Scene/Components/Render/TileComponent.h>

namespace Engine {


    Entity EditorUtils::FindEntityAtPosition(Ref<Scene> scene, const glm::vec2& worldPosition)
    {
        auto tileView = scene->GetRegistry().view<TransformComponent, TileComponent>();

        for (auto entity : tileView)
        {
            const auto& transform = tileView.get<TransformComponent>(entity);
            const auto& tileComp = tileView.get<TileComponent>(entity);

            for (size_t i = 0; i < tileComp.tiles.size(); i++)
            {
                glm::vec2 worldTilePosition = (glm::vec2)transform.Translation + tileComp.tiles[i].position;
                if (worldTilePosition == worldPosition)
                {
                    return Entity{ entity, scene.get() };
                }
            }
        }

        return Entity{};
    }
    void EditorUtils::DeleteTileAtPosition(Entity entity, const glm::vec2& worldPosition)
    {
        if (!entity.HasComponent<TransformComponent>() || !entity.HasComponent<TileComponent>())
            return;

        auto& transform = entity.GetComponent<TransformComponent>();
        auto& tileComp = entity.GetComponent<TileComponent>();

        glm::vec2 localOffset = worldPosition - glm::vec2(transform.Translation);

        auto& tiles = tileComp.tiles;
        for (auto it = tiles.begin(); it != tiles.end(); ++it)
        {
            if (glm::all(glm::epsilonEqual(it->position, localOffset, 0.01f))) // Float comparison safety
            {
                tiles.erase(it);
                EE_CORE_INFO("Tile at position ({}, {}) deleted.", localOffset.x, localOffset.y);
                return;
            }
        }

        EE_CORE_WARN("No tile found at local position ({}, {}) to delete.", localOffset.x, localOffset.y);
    }

   

}

