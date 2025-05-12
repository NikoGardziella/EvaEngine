#include "HealthSystem.h"
#include "Engine/Scene/Components/Combat/HealthComponent.h"
#include <Engine/Debug/Instrumentor.h>


void HealthSystem::UpdateHealthSystem(entt::registry& registry, float deltaTime)
{
    EE_PROFILE_FUNCTION();

    auto view = registry.view<Engine::HealthComponent>();
    for (auto entity : view)
    {
        auto& health = view.get<Engine::HealthComponent>(entity);

        if (health.Current <= 0.0f)
        {
            registry.destroy(entity);
        }
    }
}
