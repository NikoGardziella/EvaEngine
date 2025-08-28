#include "NPCAIVisionSystem.h"
#include "Engine/Scene/Components/NPC/NpcAIComponent.h"
#include "Engine/Scene/Component.h"

#include <glm/glm.hpp>
#include <Engine/Scene/Components/Player/CharacterControllerComponent.h>
#include <Engine/Debug/Instrumentor.h>
#include <Engine/Scene/Scene.h>

void NPCAIVisionSystem::UpdateNPCAIVisionSystem(float deltaTime, Engine::Scene* scene)
{
    EE_PROFILE_FUNCTION();

    scene->ForEach<NPCAIVisionComponent, NPCAIMovementComponent, Engine::TransformComponent>(
        [&](Engine::Entity npcEntity, NPCAIVisionComponent& visionComp,
            NPCAIMovementComponent& aiComp, Engine::TransformComponent& npcTransformComp)
        {
            // reset each tick
            visionComp.VisibleTarget = entt::null;

            // NPC forward direction (top-down, facing by Rotation.z)
            const float npcYaw = npcTransformComp.Rotation.z;
            const glm::vec3 npcForward = glm::normalize(glm::vec3(std::cos(npcYaw),
                std::sin(npcYaw), 0.0f));

            const float viewRadiusSq = visionComp.ViewRadius * visionComp.ViewRadius;
            const bool  limitByAngle = (visionComp.ViewAngle < 360.0f);
            const float halfFov = 0.5f * visionComp.ViewAngle;

            bool foundAny = false;

            scene->ForEach<CharacterControllerComponent, Engine::TransformComponent>(
                [&](Engine::Entity playerEntity,  CharacterControllerComponent& /*playerCtrlComp*/,
                    Engine::TransformComponent& playerTransformComp)
                {
                    if (foundAny) return; // soft "break"

                    const glm::vec3 toPlayer = playerTransformComp.Translation - npcTransformComp.Translation;
                    const float     distSq = glm::dot(toPlayer, toPlayer);
                    if (distSq > viewRadiusSq) return;

                    if (limitByAngle)
                    {
                        const glm::vec3 toPlayerDir = glm::normalize(glm::vec3(toPlayer.x, toPlayer.y, 0.0f));
                        const float     cosTheta = glm::clamp(glm::dot(npcForward, toPlayerDir), -1.0f, 1.0f);
                        const float     angleDeg = glm::degrees(std::acos(cosTheta));
                        if (angleDeg > halfFov) return;
                    }

                    // TODO: raycast / LOS check against world if needed

                    // acquire target
                    visionComp.VisibleTarget = playerEntity;
                    aiComp.CurrentState = AIState::MoveToTarget;
                    aiComp.TargetPosition = playerTransformComp.Translation;
                    foundAny = true;
                });
        });
}
