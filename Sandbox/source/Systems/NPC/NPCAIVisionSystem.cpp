#include "NPCAIVisionSystem.h"
#include "Engine/Scene/Components/NPC/NpcAIComponent.h"
#include "Engine/Scene/Component.h"

#include <glm/glm.hpp>
#include <glm/gtx/norm.hpp>
#include <Engine/Scene/Components/Player/CharacterControllerComponent.h>
#include <Engine/Debug/Instrumentor.h>

void NPCAIVisionSystem::UpdateNPCAIVisionSystem(entt::registry& registry, float deltaTime, Engine::Scene* scene)
{
    EE_PROFILE_FUNCTION();

    auto npcs = registry.view<NPCAIVisionComponent, NPCAIMovementComponent, Engine::TransformComponent>();

    for (auto npcEntity : npcs)
    {
        NPCAIVisionComponent& visionComp = registry.get<NPCAIVisionComponent>(npcEntity);
        NPCAIMovementComponent& aiComp = registry.get<NPCAIMovementComponent>(npcEntity);
        Engine::TransformComponent& npcTransformComp = registry.get<Engine::TransformComponent>(npcEntity);

        visionComp.VisibleTarget = entt::null;

        // only player has CharacterControllerComponent
        auto players = registry.view<CharacterControllerComponent, Engine::TransformComponent>();

        for (auto playerEntity : players)
        {
          
            const auto& playerTransformComp = registry.get<Engine::TransformComponent>(playerEntity);

            glm::vec3 toPlayer = playerTransformComp.Translation - npcTransformComp.Translation;
            float distSq = glm::dot(toPlayer, toPlayer);

            if (distSq > visionComp.ViewRadius * visionComp.ViewRadius)
                continue;
            

            //  check view angle
            if (visionComp.ViewAngle < 360.0f)
            {
                glm::vec3 forward = glm::vec3(0, 0, 1);
                float dot = glm::dot(glm::normalize(toPlayer), forward);
                float angle = glm::degrees(glm::acos(dot));
                if (angle > visionComp.ViewAngle * 0.5f)
                    continue;
            }

            // raycast for line of sight here
            // Found target
            visionComp.VisibleTarget = playerEntity;
            aiComp.CurrentState = AIState::MoveToTarget;
            aiComp.TargetPosition = playerTransformComp.Translation;
            break;
        }
    }
}
