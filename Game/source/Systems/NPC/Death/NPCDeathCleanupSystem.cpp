#include "NPCDeathCleanupSystem.h"
#include <Engine/Scene/Scene.h>
#include <Engine/Scene/Entity.h>
#include <Engine/Scene/Components/Render/3D/MeshRefComponent.h>
#include <Engine/Scene/Components/Render/3D/RenderBoundsComponent.h>
#include <Engine/Scene/Components/Render/3D/SkeletonComponent.h>
#include <Engine/Scene/Components/Render/3D/AnimatorComponent.h>
#include <Engine/Scene/Components/Player/CharacterControllerComponent.h>
#include <Engine/Scene/Components/NPC/NPCDeathComponent.h>
#include <Engine/Scene/Components/Combat/HealthComponent.h>
#include <Engine/Scene/Components/NPC/NpcAIComponent.h>
#include <Engine/Scene/Components/NPC/NpcAIStateComponent.h>
#include <Engine/Scene/Components/NPC/NpcBodyStateComponent.h>
#include <Engine/Scene/Components/Animation/NpcAnimationControllerComponent.h>



void NPCDeathCleanupSystem::UpdateNPCDeathCleanupSystem(float dt, Engine::Scene* scene)
{
    EE_PROFILE_FUNCTION();

    glm::vec3 playerPos(0.0f);
    bool hasPlayer = false;

    scene->ForEach<Engine::TransformComponent, CharacterControllerComponent>(
        [&](Engine::Entity e, Engine::TransformComponent& tr, CharacterControllerComponent&)
        {
            playerPos = tr.Translation;
            hasPlayer = true;
        }
    );

    // 1) Process dead NPCs
    //    - Strip gameplay components on first run
    //    - Count down timer

    std::vector<Engine::Entity> toDestroy;

    scene->ForEach<Engine::TransformComponent, NPCDeathComponent, NpcAnimationControllerComponent>(
        [&](Engine::Entity e, Engine::TransformComponent& tr, NPCDeathComponent& death, NpcAnimationControllerComponent& npcAnimationControllerComp)
        {
            if (npcAnimationControllerComp.transitionTimer > 0)
            {
                return;
            }

            // First frame after death: strip non-rendering components
            if (!death.cleanedUp)
            {
                // Remove gameplay / AI / navigation components.
                if (e.HasComponent<HealthComponent>())
                    e.RemoveComponent<HealthComponent>();

                if (e.HasComponent<NPCAIMovementComponent>())
                    e.RemoveComponent<NPCAIMovementComponent>();

                if (e.HasComponent<NPCAIVisionComponent>())
                    e.RemoveComponent<NPCAIVisionComponent>();

                if (e.HasComponent<NpcAIPatrolComponent>())
                    e.RemoveComponent<NpcAIPatrolComponent>();

                if (e.HasComponent<NpcAIStateComponent>())
                    e.RemoveComponent<NpcAIStateComponent>();

                if (e.HasComponent<NpcBodyStateComponent>())
                    e.RemoveComponent<NpcBodyStateComponent>();

                if (e.HasComponent<Engine::EnemyDestructibleComponent>())
                    e.RemoveComponent<Engine::EnemyDestructibleComponent>();

                // Note: we intentionally KEEP:
                //  - TransformComponent
                //  - MeshRefComponent
                //  - RenderBoundsComponent
                //  - SkeletonComponent
                //  - Animator3DComponent
               

                death.cleanedUp = true;
            }

            // Count down despawn timer
            death.timeToDespawn -= dt;
            if (death.timeToDespawn <= 0.0f)
            {
                toDestroy.push_back(e);
                return;
            }

            // distance-based culling
            if (death.useDistanceCulling && hasPlayer)
            {
                const float dist = glm::distance(playerPos, tr.Translation);
                if (dist > death.maxDistanceFromPlayer)
                {
                    toDestroy.push_back(e);
                    return;
                }
            }
        }
    );

    // 3) Destroy flagged entities
    for (Engine::Entity e : toDestroy)
    {
        scene->DestroyEntity(e);
    }
}

