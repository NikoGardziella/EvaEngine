#include "EditorUtils.h"
#include <Engine/Scene/Components/Render/TileComponent.h>

namespace Engine {


    Entity EditorUtils::FindTileAtPosition(Ref<Scene> scene, const glm::vec2& worldPosition)
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


   

}

