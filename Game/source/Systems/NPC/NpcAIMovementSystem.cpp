#include "NpcAIMovementSystem.h"
#include <Engine/Scene/Components/NPC/NpcAIComponent.h>

#include <glm/glm.hpp>
#include <Engine/Debug/Instrumentor.h>
#include <Engine/Scene/Scene.h>


void NpcAIMovementSystem::UpdateNPCAIMovementSystem(float deltaTime, Engine::Scene* scene)
{
    EE_PROFILE_FUNCTION();

    scene->ForEach<NPCAIMovementComponent, Engine::TransformComponent>(
        [&](Engine::Entity npcEntity, NPCAIMovementComponent& aiComp,
            Engine::TransformComponent& npcTransformComp)
        {
            glm::vec3& npcPosition = npcTransformComp.Translation;

            switch (aiComp.CurrentState)
            {
            case AIState::Idle:
            {
                aiComp.IdleTimer += deltaTime;
                if (aiComp.IdleTimer >= aiComp.IdleDuration)
                {
                    aiComp.IdleTimer = 0.0f;
                    if (!aiComp.PatrolPoints.empty())
                        aiComp.CurrentState = AIState::Patrol;
                }
            } break;

            case AIState::Patrol:
            {
                if (aiComp.PatrolPoints.empty())
                {
                    aiComp.CurrentState = AIState::Idle;
                    break;
                }

                const glm::vec3 targetPos = aiComp.PatrolPoints[aiComp.CurrentPatrolIndex];
                const glm::vec3 toTarget = targetPos - npcPosition;
                const float     dist2 = glm::dot(toTarget, toTarget);

                if (dist2 > 0.0f)
                {
                    const glm::vec3 dir = glm::normalize(toTarget);
                    npcPosition += dir * aiComp.MoveSpeed * deltaTime;
                }

                if (dist2 < 0.05f) // reached node
                {
                    aiComp.CurrentPatrolIndex =
                        (aiComp.CurrentPatrolIndex + 1) % aiComp.PatrolPoints.size();
                    aiComp.CurrentState = AIState::Idle;
                    aiComp.IdleTimer = 0.0f;
                }
            } break;

            case AIState::MoveToTarget:
            {
                const glm::vec3 toTarget = aiComp.TargetPosition - npcPosition;
                const float     dist2 = glm::dot(toTarget, toTarget);

                if (dist2 < 1.1f)
                {
                    aiComp.CurrentState = AIState::Idle;
                    aiComp.IdleTimer = 0.0f;
                    break;
                }

                // Move
                glm::vec3 dir = glm::normalize(toTarget);
                npcPosition += dir * aiComp.MoveSpeed * deltaTime;

                // Smooth face direction (top-down sprite that looks “up” by default)
                const float currentAngle = npcTransformComp.Rotation.z;
                float targetAngle = std::atan2(dir.y, dir.x) - glm::radians(90.0f);

                float delta = targetAngle - currentAngle;
                delta = std::atan2(std::sin(delta), std::cos(delta)); // wrap to [-pi, pi]

                constexpr float kRotationSpeed = 5.0f; // radians/sec
                const float maxStep = kRotationSpeed * deltaTime;
                delta = glm::clamp(delta, -maxStep, maxStep);

                npcTransformComp.Rotation.z = currentAngle + delta;
            } break;
            }
        });
}
