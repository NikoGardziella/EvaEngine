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

        const float projectileSpeed = 10.0f;

        // Move projectile
        projectileTransform.Translation.x += projectile.Velocity.x * deltaTime * projectileSpeed;
        projectileTransform.Translation.y += projectile.Velocity.y * deltaTime * projectileSpeed;

        glm::vec2 projPos = {
            projectileTransform.Translation.x,
            projectileTransform.Translation.y
        };

        // Check collisions
        auto targetView = registry.view<Engine::TransformComponent>();

        for (auto targetEntity : targetView)
        {
            if (targetEntity == projectileEntity)
                continue;

            const auto& targetTransform = targetView.get<Engine::TransformComponent>(targetEntity);
            glm::vec2 targetPos = {
                targetTransform.Translation.x,
                targetTransform.Translation.y
            };

            bool hit = false;

            // Box collider check
            if (registry.any_of<Engine::BoxCollider2DComponent>(targetEntity))
            {
                const auto& box = registry.get<Engine::BoxCollider2DComponent>(targetEntity);
                glm::vec2 boxSize = box.Size;

                glm::vec2 min = targetPos - boxSize * 0.5f;
                glm::vec2 max = targetPos + boxSize * 0.5f;

                if (projPos.x >= min.x && projPos.x <= max.x &&
                    projPos.y >= min.y && projPos.y <= max.y)
                {
                    hit = true;
                }
            }

            // Circle collider check
            if (!hit && registry.any_of<Engine::CircleCollider2DComponent>(targetEntity))
            {
                const auto& circle = registry.get<Engine::CircleCollider2DComponent>(targetEntity);
                float radius = circle.Radius;

                float distSq = glm::distance2(projPos, targetPos);
                if (distSq <= radius * radius)
                {
                    hit = true;
                }
            }

            if (hit)
            {
                // Apply damage if target has HealthComponent
                if (registry.any_of<Engine::HealthComponent>(targetEntity))
                {
                    auto& health = registry.get<Engine::HealthComponent>(targetEntity);
                    health.Current -= projectile.Damage;
                }

                // Destroy projectile on hit
                registry.destroy(projectileEntity);
                break;
            }
        }

        // Expire projectile by lifetime
        if (registry.valid(projectileEntity)) // Check in case it was destroyed above
        {
            projectile.LifeTime -= deltaTime;
            if (projectile.LifeTime <= 0.0f)
            {
                registry.destroy(projectileEntity);
            }
        }
    }
}
