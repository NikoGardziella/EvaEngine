#include "NpcAIMovementSystem.h"
#include <Engine/Scene/Components/NPC/NpcAIComponent.h>

#include <glm/glm.hpp>
#include <Engine/Debug/Instrumentor.h>
#include <Engine/Scene/Scene.h>
#include <Engine/Map/Grid/GridMap.h>
#include <Engine/Core/Core.h>
#include <Engine/Scene/Components/Render/3D/AnimatorComponent.h>
#include <Engine/AssetManager/AssetManager.h>

void NpcAIMovementSystem::UpdateNPCAIMovementSystem(float deltaTime, Engine::Scene* scene)
{
    EE_PROFILE_FUNCTION();

    // 1) Gather agents once
    std::vector<AgentRef> agents;
    agents.reserve(128);

    scene->ForEach<Engine::TransformComponent, NPCAIMovementComponent>(
        [&](Engine::Entity e, Engine::TransformComponent& tr, NPCAIMovementComponent& ai)
        {
            AgentRef a;
            a.tr = &tr;          // store pointer
            a.radius = ai.radius;
            agents.push_back(a);
        });

   
    // keep them away from each other.
    const size_t count = agents.size();
    for (size_t i = 0; i < count; ++i)
    {
        AgentRef& a = agents[i];

        glm::vec2 p(a.tr->Translation.x, a.tr->Translation.y);
        glm::vec2 separation(0.0f);
        int sepCount = 0;

        for (size_t j = 0; j < count; ++j)
        {
            if (i == j) continue;

            AgentRef& b = agents[j];
            glm::vec2 q(b.tr->Translation.x, b.tr->Translation.y);

            glm::vec2 diff = p - q;
            float dist2 = glm::dot(diff, diff);
            float minDist = a.radius + b.radius;
            float minDist2 = minDist * minDist;

            if (dist2 < minDist2 && dist2 > 1e-6f)
            {
                float dist = glm::sqrt(dist2);
                glm::vec2 dir = diff / dist;
                float push = (minDist - dist);
                separation += dir * push;
                ++sepCount;
            }
        }

        if (sepCount > 0)
        {
            separation /= float(sepCount);
            separation *= 0.5f; // tweak factor

            a.tr->Translation.x += separation.x;
            a.tr->Translation.y += separation.y;
        }
    }


    scene->ForEach<NPCAIMovementComponent, Engine::TransformComponent>(
        [&](Engine::Entity npcEntity, NPCAIMovementComponent& aiComp, Engine::TransformComponent& npcTransformComp)
        {
            glm::vec3& npcPosition = npcTransformComp.Translation;
            Engine::Ref<Engine::GridMap>& grid = scene->GetGrid();

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
                        Engine::Animator3DComponent& npcAnimComp = npcEntity.GetComponent<Engine::Animator3DComponent>();
                        Engine::AnimationRegistry& animReg = Engine::AssetManager::GetAnimationRegistry();
                        npcAnimComp.clipA = animReg.FindAnimationClip("zombieAnimRun")->id;

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
                    Engine::Animator3DComponent& npcAnimComp = npcEntity.GetComponent<Engine::Animator3DComponent>();
                    Engine::AnimationRegistry& animReg = Engine::AssetManager::GetAnimationRegistry();
                    npcAnimComp.clipA = animReg.FindAnimationClip("zombieAnimIdle")->id;

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

                if (dist2 < 0.05f)
                {
                    aiComp.CurrentPatrolIndex =
                        (aiComp.CurrentPatrolIndex + 1) % aiComp.PatrolPoints.size();
                    aiComp.CurrentState = AIState::Idle;
                    aiComp.IdleTimer = 0.0f;
                }
                break;
            }

            case AIState::MoveToTarget:
            {
                // current live target pos (player)
                glm::vec2 start2D(npcPosition.x, npcPosition.y);
                glm::vec2 liveTarget2D(aiComp.TargetPosition.x, aiComp.TargetPosition.y);

                // If we have LOS again -> go back to ChaseLOS
                if (grid->HasLineOfSight(start2D, liveTarget2D, false))
                {
                    aiComp.LastKnownTargetPos = aiComp.TargetPosition;
                    aiComp.HasLastKnownTarget = true;
                    aiComp.CurrentState = AIState::ChaseLOS;
                    aiComp.IdleTimer = 0.0f;
                    aiComp.ClearPath();


                    Engine::Animator3DComponent& npcAnimComp = npcEntity.GetComponent<Engine::Animator3DComponent>();
                    Engine::AnimationRegistry& animReg = Engine::AssetManager::GetAnimationRegistry();
                    npcAnimComp.clipA = animReg.FindAnimationClip("zombieAnimRun")->id;
                    break;
                }

                // Use last known position as goal if we have one, otherwise live target
                glm::vec3 goalPos = aiComp.HasLastKnownTarget
                    ? aiComp.LastKnownTargetPos
                    : aiComp.TargetPosition;

                glm::vec2 goal2D(goalPos.x, goalPos.y);

                // Build path if we don't have one
                if (!aiComp.HasPath())
                {
                    std::vector<glm::vec2> path2D = grid->FindPathWorld(start2D, goal2D);

                    aiComp.Path.clear();
                    aiComp.Path.reserve(path2D.size());
                    for (auto& p : path2D)
                        aiComp.Path.push_back(glm::vec3(p.x, p.y, npcPosition.z));

                    aiComp.PathIndex = 0;
                }

                // No valid path -> give up and idle
                if (!aiComp.HasPath())
                {
                    aiComp.CurrentState = AIState::Idle;
                    aiComp.IdleTimer = 0.0f;
                    Engine::Animator3DComponent& npcAnimComp = npcEntity.GetComponent<Engine::Animator3DComponent>();
                    Engine::AnimationRegistry& animReg = Engine::AssetManager::GetAnimationRegistry();
                    npcAnimComp.clipA = animReg.FindAnimationClip("zombieAnimIdle")->id;
                    break;
                }

                // Follow path
                glm::vec3 waypoint = aiComp.Path[aiComp.PathIndex];
                glm::vec3 toTarget = waypoint - npcPosition;
                float     dist2 = glm::dot(toTarget, toTarget);

                const float reachRadius = 0.05f;
                if (dist2 < reachRadius * reachRadius)
                {
                    aiComp.PathIndex++;
                    if (!aiComp.HasPath())
                    {
                        // Reached last known position
                        aiComp.CurrentState = AIState::Idle; // or a Search state later
                        aiComp.IdleTimer = 0.0f;
                        aiComp.ClearPath();
                        Engine::Animator3DComponent& npcAnimComp = npcEntity.GetComponent<Engine::Animator3DComponent>();
                        Engine::AnimationRegistry& animReg = Engine::AssetManager::GetAnimationRegistry();
                        npcAnimComp.clipA = animReg.FindAnimationClip("zombieAnimIdle")->id;
                        break;
                    }

                    waypoint = aiComp.Path[aiComp.PathIndex];
                    toTarget = waypoint - npcPosition;
                    dist2 = glm::dot(toTarget, toTarget);
                }

                if (dist2 > 0.0f)
                {
                    glm::vec3 dir = glm::normalize(toTarget);
                    npcPosition += dir * aiComp.MoveSpeed * deltaTime;

                    const float currentAngle = npcTransformComp.Rotation.z;
                    float targetAngle = std::atan2(dir.y, dir.x) + glm::radians(90.0f);

                    float delta = targetAngle - currentAngle;
                    delta = std::atan2(std::sin(delta), std::cos(delta));

                    constexpr float kRotationSpeed = 5.0f;
                    const float maxStep = kRotationSpeed * deltaTime;
                    delta = glm::clamp(delta, -maxStep, maxStep);

                    npcTransformComp.Rotation.z = currentAngle + delta;
                }

                break;
            }

            case AIState::ChaseLOS:
            {
                glm::vec2 npcPos2(npcPosition.x, npcPosition.y);
                glm::vec2 targetPos2(aiComp.TargetPosition.x, aiComp.TargetPosition.y);

                // If LOS is still valid, update last known position while chasing
                if (grid->HasLineOfSight(npcPos2, targetPos2, false))
                {
                    aiComp.LastKnownTargetPos = aiComp.TargetPosition;
                    aiComp.HasLastKnownTarget = true;

                    glm::vec3 toTarget = aiComp.TargetPosition - npcPosition;
                    float     dist2 = glm::dot(toTarget, toTarget);

                    const float kStopDistance = 0.5f;
                    if (dist2 < kStopDistance * kStopDistance)
                    {
                        aiComp.CurrentState = AIState::Attack;
                        aiComp.IdleTimer = 0.0f;
                        break;
                    }

                    glm::vec3 dir = glm::normalize(toTarget);
                    npcPosition += dir * aiComp.MoveSpeed * deltaTime;

                    const float currentAngle = npcTransformComp.Rotation.z;
                    float targetAngle = std::atan2(dir.y, dir.x) + glm::radians(90.0f);

                    float delta = targetAngle - currentAngle;
                    delta = std::atan2(std::sin(delta), std::cos(delta));

                    constexpr float kRotationSpeed = 5.0f;
                    const float maxStep = kRotationSpeed * deltaTime;
                    delta = glm::clamp(delta, -maxStep, maxStep);

                    npcTransformComp.Rotation.z = currentAngle + delta;
                }
                else
                {
                    // Lost LOS -> go pathfind to last known spot
                    if (aiComp.HasLastKnownTarget)
                    {
                        aiComp.CurrentState = AIState::MoveToTarget;
                        aiComp.ClearPath();
                    }
                    else
                    {
                        // Never saw the target properly -> just idle
                        aiComp.CurrentState = AIState::Idle;
                        aiComp.IdleTimer = 0.0f;

                        Engine::Animator3DComponent& npcAnimComp = npcEntity.GetComponent<Engine::Animator3DComponent>();
                        Engine::AnimationRegistry& animReg = Engine::AssetManager::GetAnimationRegistry();
                        npcAnimComp.clipA = animReg.FindAnimationClip("zombieAnimIdle")->id;
                    }
                }

                break;
            }

            case AIState::Attack:
            {
                EE_INFO("attack");
                aiComp.CurrentState = AIState::Idle;
                break;
            }
            }
        });
}
