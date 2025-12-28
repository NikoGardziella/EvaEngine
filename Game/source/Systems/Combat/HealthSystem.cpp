#include "HealthSystem.h"
#include "Engine/Scene/Components/Combat/HealthComponent.h"
#include <Engine/Debug/Instrumentor.h>
#include <Engine/Scene/Scene.h>
#include <Engine/Scene/Components/Animation/NpcAnimationControllerComponent.h>
#include <Engine/Scene/Components/NPC/NpcBodyStateComponent.h>
#include <Engine/Scene/Components/NPC/NPCDeathComponent.h>


void HealthSystem::UpdateHealthSystem(float deltaTime, Engine::Scene* scene)
{
    EE_PROFILE_FUNCTION();

    scene->ForEach<HealthComponent>(
        [scene](Engine::Entity e, HealthComponent& health)
        {
            if (health.Current <= 0.0f)
            {
                NpcAIStateComponent& npcStateComponent = e.GetComponent<NpcAIStateComponent>();
                if (npcStateComponent.state == AIState::Dead && !e.HasComponent<NPCDeathComponent>())
                {
                    // make new system to deal with removing entities
                    NPCDeathComponent& death = e.AddComponent<NPCDeathComponent>();
                    death.timeToDespawn = 10.0f;
                    death.maxDistanceFromPlayer = 60.0f; 
                }


            }
        });
}
