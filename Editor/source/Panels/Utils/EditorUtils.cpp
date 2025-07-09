#include "EditorUtils.h"
#include <Engine/Scene/Components/Render/TileComponent.h>

namespace Engine {


    Entity EditorUtils::FindTileAtPosition(Ref<Scene> scene, const glm::vec2& worldPosition)
    {
        auto tileView = scene->GetRegistry().view<TileComponent>();


        for (auto entity : tileView)
        {
            const auto& tileComp = tileView.get<TileComponent>(entity);
            for (size_t i = 0; i < tileComp.tiles.size(); i++)
            {
                glm::vec2 tilePosition = tileComp.tiles[i].position;
                if (tilePosition == worldPosition)
                {
                    return Entity{ entity, scene.get() };
                }
            }
            
        }

        return Entity{};
    }

   

}

