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


            uint32_t pixelSize = 1;
            // World units per pixel
            float pixelWidth = pixelTransform.Scale.x / pixelTexture.Texture->GetWidth();
            float pixelHeight = pixelTransform.Scale.y / pixelTexture.Texture->GetHeight();

            glm::vec2 relative = playerPos - glm::vec2(pixelTransform.Translation.x, pixelTransform.Translation.y);

            int px = static_cast<int>(relative.x / pixelWidth);
            int py = static_cast<int>(relative.y / pixelHeight);
            py = pixelTexture.Texture->GetHeight() - 1 - py; // flip Y if needed


            if (pixelTexture.Texture != nullptr)
            {
                if (px >= 0 && py >= 0 && px < pixelTexture.Texture->GetWidth() && py < pixelTexture.Texture->GetHeight())
                {
                    // Destroy pixel on collision
                    pixelTexture.Texture->DestroyPixel(px, py);
					pixelTexture.Texture->ApplyChanges();
                }
            }

            
        }
    }
}
