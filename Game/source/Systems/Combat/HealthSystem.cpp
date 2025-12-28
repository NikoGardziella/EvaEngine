#include "HealthSystem.h"
#include "Engine/Scene/Components/Combat/HealthComponent.h"
#include <Engine/Debug/Instrumentor.h>
#include <Engine/Scene/Scene.h>
#include <Engine/Scene/Components/Animation/NpcAnimationControllerComponent.h>
#include <Engine/Scene/Components/NPC/NpcBodyStateComponent.h>


void HealthSystem::UpdateHealthSystem(float deltaTime, Engine::Scene* scene)
{
    EE_PROFILE_FUNCTION();

    scene->ForEach<Engine::HealthComponent>(
        [scene](Engine::Entity e, Engine::HealthComponent& health)
        {
            if (health.Current <= 0.0f)
            {
                NpcAnimationControllerComponent& npcAnimationControllerComp = e.GetComponent<NpcAnimationControllerComponent>();
                NpcAIStateComponent& npcStateComponent = e.GetComponent<NpcAIStateComponent>();
                if (npcAnimationControllerComp.transitionTimer <= -5.0f && npcStateComponent.state == AIState::Dead)
                {
                    // make new system to deal with removing entities
                    scene->DestroyEntity(e);
                }


            }
        });
}
