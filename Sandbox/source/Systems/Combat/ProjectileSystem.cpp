#include "ProjectileSystem.h"
#include <Engine/Debug/Instrumentor.h>



void ProjectileSystem::UpdateProjectileSystem(entt::registry& registry, float deltaTime)
{
    EE_PROFILE_FUNCTION();


    auto view = registry.view<Engine::TransformComponent, Engine::ProjectileComponent>();
    for (auto entity : view)
    {
        auto& transform = view.get<Engine::TransformComponent>(entity);
        auto& projectile = view.get<Engine::ProjectileComponent>(entity);

        float projectileSpeed = 10.0f;

        // Move
        transform.Translation.x += projectile.Velocity.x * deltaTime * projectileSpeed;
        transform.Translation.y += projectile.Velocity.y * deltaTime * projectileSpeed;

        // Countdown lifetime
        projectile.LifeTime -= deltaTime;
        if (projectile.LifeTime <= 0.0f)
        {
            registry.destroy(entity);
        }
    }
}