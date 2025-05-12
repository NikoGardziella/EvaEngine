#include "ProjectileSystem.h"
#include <Engine/Debug/Instrumentor.h>
#include <Engine/Scene/Components/Combat/HealthComponent.h>



void ProjectileSystem::UpdateProjectileSystem(entt::registry& registry, float deltaTime)
{
    EE_PROFILE_FUNCTION();

    auto projectileView = registry.view<Engine::TransformComponent, Engine::ProjectileComponent>();

    for (auto projectileEntity : projectileView)
    {
        auto& projectileTransform = projectileView.get<Engine::TransformComponent>(projectileEntity);
        auto& projectile = projectileView.get<Engine::ProjectileComponent>(projectileEntity);

        float projectileSpeed = 10.0f;

        // Move
        projectileTransform.Translation.x += projectile.Velocity.x * deltaTime * projectileSpeed;
        projectileTransform.Translation.y += projectile.Velocity.y * deltaTime * projectileSpeed;

        // Collision detection
        auto colliderView = registry.view<Engine::TransformComponent, Engine::HealthComponent>();
        for (auto targetEntity : colliderView)
        {
            if (targetEntity == projectileEntity)
                continue;

            auto& targetTransform = colliderView.get<Engine::TransformComponent>(targetEntity);
            bool hit = false;

            // Box collider check
            if (registry.any_of<Engine::BoxCollider2DComponent>(targetEntity))
            {
                auto& box = registry.get<Engine::BoxCollider2DComponent>(targetEntity);

                glm::vec2 boxPos = glm::vec2(targetTransform.Translation.x, targetTransform.Translation.y);
                glm::vec2 boxSize = box.Size;

                glm::vec2 projPos = glm::vec2(projectileTransform.Translation.x, projectileTransform.Translation.y);

                glm::vec2 min = boxPos - boxSize * 0.5f;
                glm::vec2 max = boxPos + boxSize * 0.5f;

                if (projPos.x >= min.x && projPos.x <= max.x &&
                    projPos.y >= min.y && projPos.y <= max.y)
                {
                    hit = true;
                }
            }

            // Circle collider check
            if (!hit && registry.any_of<Engine::CircleCollider2DComponent>(targetEntity))
            {
                auto& circle = registry.get<Engine::CircleCollider2DComponent>(targetEntity);
                glm::vec2 center = glm::vec2(targetTransform.Translation.x, targetTransform.Translation.y);
                float radius = circle.Radius;

                glm::vec2 projPos = glm::vec2(projectileTransform.Translation.x, projectileTransform.Translation.y);

                float distSq = glm::distance2(center, projPos);
                if (distSq <= radius * radius)
                {
                    hit = true;
                }
            }

            if (hit)
            {
                Engine::HealthComponent& health = colliderView.get<Engine::HealthComponent>(targetEntity);
                health.Current -= projectile.Damage;

                registry.destroy(projectileEntity); // Destroy projectile on hit
                break; // Only hit one target
            }
        }

        // Countdown lifetime
        projectile.LifeTime -= deltaTime;
        if (projectile.LifeTime <= 0.0f)
        {
            registry.destroy(projectileEntity);
        }
    }
}
