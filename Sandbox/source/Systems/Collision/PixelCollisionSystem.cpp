#include "PixelCollisionSystem.h"
#include "glm/glm.hpp"
#include <Engine/Debug/Instrumentor.h>


void PixelCollisionSystem::UpdatePixelCollisionSystem(entt::registry& registry, float deltaTime)
{
    EE_PROFILE_FUNCTION();
    auto playerView = registry.view<Engine::TransformComponent, Engine::CharacterControllerComponent>();
    auto pixelView = registry.view<Engine::TransformComponent, Engine::PixelSpriteRendererComponent>();

    for (auto playerEntity : playerView)
    {
        auto& playerTransform = playerView.get<Engine::TransformComponent>(playerEntity);
        glm::vec2 playerPos = glm::vec2(playerTransform.Translation.x, playerTransform.Translation.y);

        for (auto pixelEntity : pixelView)
        {
            auto& pixelTransform = pixelView.get<Engine::TransformComponent>(pixelEntity);
            Engine::PixelSpriteRendererComponent& pixelTexture = pixelView.get<Engine::PixelSpriteRendererComponent>(pixelEntity);

            glm::vec2 pixelOrigin = pixelTransform.Translation;
            glm::vec2 relative = playerPos - pixelOrigin;

            uint32_t pixelSize = 1;

            int px = static_cast<int>(relative.x / pixelSize);
            int py = static_cast<int>(relative.y / pixelSize);

            if (pixelTexture.Texture != nullptr)
            {
                if (px >= 0 && py >= 0 && px < pixelTexture.Texture->GetWidth() && py < pixelTexture.Texture->GetHeight())
                {
                    // Destroy pixel on collision
                    pixelTexture.Texture->DestroyPixel(px, py);

                }
            }

            
        }
    }
}
