#include "EditorUtils.h"
#include <Engine/Scene/Components/Render/TileComponent.h>

namespace Engine {


    Entity EditorUtils::FindTileAtPosition(Ref<Scene> scene, const glm::vec2& worldPosition)
    {
        auto tileView = scene->GetRegistry().view<TileComponent>();

        for (auto entity : tileView)
        {
            const auto& tileComp = tileView.get<TileComponent>(entity);

            glm::vec2 tilePosition = tileComp.WorldPos;
            if (tilePosition == worldPosition)
            {
                return Entity{entity, scene.get()};
            }
        }

        return Entity{};
    }

   

}

