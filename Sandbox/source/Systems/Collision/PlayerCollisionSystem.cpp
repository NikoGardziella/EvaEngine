#include "PlayerCollisionSystem.h"
#include "Engine/Scene/Scene.h"

void PlayerCollisionSystem::UpdatePlayerCollision(entt::registry& registry, float deltaTime)
{
    /*
    // For simplicity, assume a single player entity with both Transform and PixelCollider
    auto view = registry.view<Engine::TransformComponent, Engine::CharacterControllerComponent, Engine::CircleCollider2DComponent>();
    for (auto entity : view)
    {
        auto& transform = view.get<Engine::TransformComponent>(entity);
        auto& controller = view.get<Engine::CharacterControllerComponent>(entity);
        auto& collider = view.get<Engine::CircleCollider2DComponent>(entity);
        auto texture = collider.Texture;

        // Compute the player's new position if moved
        glm::vec2 newPos = glm::vec2(transform.Translation.x, transform.Translation.y)
            + controller.velocity * deltaTime;

        // Convert world position to texture pixel space
        // Assume 1 world unit = 1 pixel, and origin aligned
        int baseX = (int)std::floor(newPos.x) + collider.Offset.x;
        int baseY = (int)std::floor(newPos.y) + collider.Offset.y;

        // Check pixel region overlap for solidity
        bool collision = false;
        int w = texture.ra;
        int h = texture->GetHeight();

        // Loop over collider texture pixels
        for (int py = 0; py < h && !collision; ++py)
        {
            for (int px = 0; px < w; ++px)
            {
                if (texture->IsPixelSolid(px, py))
                {
                    int worldX = baseX + px;
                    int worldY = baseY + py;
                    // If this pixel is solid, we have collision
                    // Here you could also test against a map or other collider
                    collision = true;
                    break;
                }
            }
        }

        if (collision)
        {
            // On collision, stop movement or slide
            controller.velocity = glm::vec2(0.0f);
        }
        else 
        {
            // No collision: apply movement
            transform.Translation.x = newPos.x;
            transform.Translation.y = newPos.y;
        }
    }
    */
}