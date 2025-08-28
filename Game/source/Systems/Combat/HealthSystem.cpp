#include "HealthSystem.h"
#include "Engine/Scene/Components/Combat/HealthComponent.h"
#include <Engine/Debug/Instrumentor.h>
#include <Engine/Scene/Scene.h>


void HealthSystem::UpdateHealthSystem(float deltaTime, Engine::Scene* scene)
{
    EE_PROFILE_FUNCTION();

    scene->ForEach<Engine::HealthComponent>(
        [scene](Engine::Entity e, Engine::HealthComponent& health)
        {
            if (health.Current <= 0.0f)
            {
                scene->DestroyEntity(e);
            }
        });
}
