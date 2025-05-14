#include "NpcAIMovementSystem.h"
#include <Engine/Scene/Components/NPC/NpcAIComponent.h>

#include <glm/glm.hpp>
#include <glm/gtx/norm.hpp>

void NpcAIMovementSystem::UpdateNPCAIMovementSystem(entt::registry& registry, float deltaTime, Engine::Scene* scene)
{
    auto view = registry.view<NPCAIMovementComponent, Engine::TransformComponent>();

    for (auto entity : view)
    {
        NPCAIMovementComponent& aiComp = view.get<NPCAIMovementComponent>(entity);
        Engine::TransformComponent& NPCTransformComp = view.get<Engine::TransformComponent>(entity);
        glm::vec3& NPCpos = NPCTransformComp.Translation;

        switch (aiComp.CurrentState)
        {
        case AIState::Idle:
        {
            aiComp.IdleTimer += deltaTime;
            if (aiComp.IdleTimer >= aiComp.IdleDuration)
            {
                aiComp.IdleTimer = 0.0f;

                if (!aiComp.PatrolPoints.empty())
                {
                    aiComp.CurrentState = AIState::Patrol;
                }
            }
            break;
        }

        case AIState::Patrol:
        {
            if (aiComp.PatrolPoints.empty())
            {
                aiComp.CurrentState = AIState::Idle;
                break;
            }

            glm::vec3 target = aiComp.PatrolPoints[aiComp.CurrentPatrolIndex];
            glm::vec3 direction = glm::normalize(target - NPCpos);
            NPCpos += direction * aiComp.MoveSpeed * deltaTime;

            if (glm::distance2(NPCpos, target) < 0.05f)
            {
                aiComp.CurrentPatrolIndex = (aiComp.CurrentPatrolIndex + 1) % aiComp.PatrolPoints.size();
                aiComp.CurrentState = AIState::Idle;
                aiComp.IdleTimer = 0.0f;
            }
            break;
        }

        case AIState::MoveToTarget:
        {


            glm::vec3 direction = glm::normalize(aiComp.TargetPosition - NPCpos);
            NPCpos += direction * aiComp.MoveSpeed * deltaTime;

            float currentAngle = NPCTransformComp.Rotation.z;
            float targetAngle = std::atan2(direction.y, direction.x);
            float rotationFix = glm::radians(90.0f); // convert degrees to radians
            targetAngle -= rotationFix;

            float delta = targetAngle - currentAngle;
            delta = std::atan2(std::sin(delta), std::cos(delta)); 

            float rotationSpeed = 5.0f; // radians per second
            float maxStep = rotationSpeed * deltaTime;
            delta = glm::clamp(delta, -maxStep, maxStep);

            NPCTransformComp.Rotation.z = currentAngle + delta;


            if (glm::distance2(NPCpos, aiComp.TargetPosition) < 0.05f)
            {
                aiComp.CurrentState = AIState::Idle;
                aiComp.IdleTimer = 0.0f;
            }
            break;
        }
        }
    }
}
