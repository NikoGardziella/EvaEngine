#include "NpcAIMovementSystem.h"
#include <Engine/Scene/Components/NPC/NpcAIComponent.h>

#include <glm/glm.hpp>
#include <Engine/Debug/Instrumentor.h>
#include <Engine/Scene/Scene.h>
#include <Engine/Map/Grid/GridMap.h>
#include <Engine/Core/Core.h>
#include <Engine/Scene/Components/Render/3D/AnimatorComponent.h>
#include <Engine/AssetManager/AssetManager.h>
#include <Engine/Scene/Components/Animation/NpcAnimationControllerComponent.h>

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

    scene->ForEach<NPCAIMovementComponent, Engine::TransformComponent, NpcAnimationControllerComponent>(
        [&](Engine::Entity npcEntity, NPCAIMovementComponent& aiComp, Engine::TransformComponent& tr, NpcAnimationControllerComponent& animCtrl)
        {
            Engine::Ref<Engine::GridMap>& grid = scene->GetGrid();
            grid->DebugDrawPath(aiComp.Path);

            // ---- One-shot action wait (Attack/Hit/Death etc) ----
            if (aiComp.CurrentState == AIState::Attack)
            {
                // Controller owns timing. AI just waits.
                if (animCtrl.actionTimer > 0.0f)
                    return;

                aiComp.CurrentState = animCtrl.returnState;
            }

            switch (aiComp.CurrentState)
            {
            case AIState::Idle:
            {
                if (HasLOSNow(grid, tr.Translation, aiComp.TargetPosition))
                {
                    EnterChaseLOS(aiComp);
                    break;
                }

                aiComp.IdleTimer += deltaTime;
                if (aiComp.IdleTimer >= aiComp.IdleDuration)
                {
                    aiComp.IdleTimer = 0.0f;
                    if (!aiComp.PatrolPoints.empty())
                        aiComp.CurrentState = AIState::Patrol;
                }
                break;
            }

            case AIState::Patrol:
            {
                if (HasLOSNow(grid, tr.Translation, aiComp.TargetPosition))
                {
                    EnterChaseLOS(aiComp);
                    break;
                }

                if (aiComp.PatrolPoints.empty())
                {
                    EnterIdle(aiComp);
                    break;
                }

                glm::vec3 target = aiComp.PatrolPoints[aiComp.CurrentPatrolIndex];
                glm::vec3 to = target - tr.Translation;
                float dist2 = glm::dot(to, to);

                const float reach = 0.10f;
                if (dist2 <= reach * reach)
                {
                    aiComp.CurrentPatrolIndex = (aiComp.CurrentPatrolIndex + 1) % aiComp.PatrolPoints.size();
                    EnterIdle(aiComp);
                    break;
                }

                if (dist2 > 1e-8f)
                {
                    glm::vec3 dir = to / std::sqrt(dist2);
                    tr.Translation += dir * aiComp.MoveSpeed * deltaTime;
                    RotateTowardsDirXY(tr, dir, deltaTime);
                }
                break;
            }

            case AIState::ChaseLOS:
            {
                if (HasLOSNow(grid, tr.Translation, aiComp.TargetPosition))
                {
                    aiComp.LastKnownTargetPos = aiComp.TargetPosition;
                    aiComp.HasLastKnownTarget = true;

                    glm::vec3 to = aiComp.TargetPosition - tr.Translation;
                    float dist2 = glm::dot(to, to);

                    const float stop = 0.5f;
                    if (dist2 <= stop * stop)
                    {
                        aiComp.CurrentState = AIState::Attack;
                       // BeginOneShotAction(aiComp, animCtrl, AIState::Attack, 0.8f, AIState::ChaseLOS);
                        break;
                    }

                    if (dist2 > 1e-8f)
                    {
                        glm::vec3 dir = to / std::sqrt(dist2);
                        tr.Translation += dir * aiComp.MoveSpeed * deltaTime;
                        RotateTowardsDirXY(tr, dir, deltaTime);
                    }
                }
                else
                {
                    if (aiComp.HasLastKnownTarget) EnterMoveToLastKnown(aiComp);
                    else EnterIdle(aiComp);
                }
                break;
            }

            case AIState::MoveToTarget:
            {
                if (HasLOSNow(grid, tr.Translation, aiComp.TargetPosition))
                {
                    EnterChaseLOS(aiComp);
                    break;
                }

                if (!aiComp.HasLastKnownTarget)
                {
                    EnterIdle(aiComp);
                    break;
                }

                // fixed goal while LOS missing
                glm::vec2 goal2(aiComp.LastKnownTargetPos.x, aiComp.LastKnownTargetPos.y);
                glm::vec2 npc2(tr.Translation.x, tr.Translation.y);

                // repath throttling
                aiComp.RepathTimer += deltaTime;

                const float repathStartMoveThresh = 0.25f;
                bool startMoved = glm::length(npc2 - aiComp.LastRepathStart2D) > repathStartMoveThresh;

                bool needRepath = (!aiComp.HasPath()) ||
                    (aiComp.RepathTimer >= aiComp.RepathInterval && startMoved);

                if (needRepath)
                {
                    std::vector<glm::vec2> path2D = grid->FindPathWorld(npc2, goal2);

                    aiComp.Path.clear();
                    aiComp.Path.reserve(path2D.size());
                    for (const auto& p : path2D)
                        aiComp.Path.push_back(glm::vec3(p.x, p.y, tr.Translation.z));

                    aiComp.PathIndex = 0;
                    aiComp.RepathTimer = 0.0f;
                    aiComp.LastRepathStart2D = npc2;
                    aiComp.LastRepathGoal2D = goal2;
                }

                if (!aiComp.HasPath())
                {
                    EnterIdle(aiComp);
                    break;
                }

                bool moving = MoveAlongPathXY(aiComp, tr, deltaTime);
                if (!moving)
                {
                    aiComp.HasLastKnownTarget = false;
                    EnterIdle(aiComp);
                }
                break;
            }

            case AIState::Attack:
            {
                // If attack forced externally without setting timers, make it safe.
                if (animCtrl.actionTimer <= 0.0f)
                    BeginOneShotAction(aiComp, animCtrl, AIState::Attack, 0.8f, AIState::ChaseLOS);
                break;
            }

            default:
                break;
            }
        });




}


    void NpcAIMovementSystem::RotateTowardsDirXY(Engine::TransformComponent& tr, const glm::vec3& dir, float dt)
    {
        // XY plane
        const float cur = tr.Rotation.z;
        float tgt = std::atan2(dir.y, dir.x) + glm::radians(90.0f);

        float da = tgt - cur;
        da = std::atan2(std::sin(da), std::cos(da));

        constexpr float rotSpeed = 5.0f;
        float maxStep = rotSpeed * dt;
        da = glm::clamp(da, -maxStep, maxStep);

        tr.Rotation.z = cur + da;
    }

    bool NpcAIMovementSystem::HasLOSNow(const Engine::Ref<Engine::GridMap>& grid, const glm::vec3& npcPos3, const glm::vec3& targetPos3)
    {
        glm::vec2 npc2(npcPos3.x, npcPos3.y);
        glm::vec2 tgt2(targetPos3.x, targetPos3.y);
        return grid->HasLineOfSight(npc2, tgt2, false);
    }

    void NpcAIMovementSystem::EnterIdle(NPCAIMovementComponent& ai)
    {
        ai.CurrentState = AIState::Idle;
        ai.IdleTimer = 0.0f;
        ai.ClearPath();
        ai.RepathTimer = 0.0f;
    }

    void NpcAIMovementSystem::EnterChaseLOS(NPCAIMovementComponent& ai)
    {
        ai.LastKnownTargetPos = ai.TargetPosition;
        ai.HasLastKnownTarget = true;
        ai.CurrentState = AIState::ChaseLOS;
        ai.IdleTimer = 0.0f;
        ai.ClearPath();
        ai.RepathTimer = 0.0f;
    }

    void NpcAIMovementSystem::EnterMoveToLastKnown(NPCAIMovementComponent& ai)
    {
        ai.CurrentState = AIState::MoveToTarget;
        ai.IdleTimer = 0.0f;
        ai.ClearPath();
        ai.RepathTimer = ai.RepathInterval; // force immediate path build
    }

    void NpcAIMovementSystem::BeginOneShotAction(NPCAIMovementComponent& ai, NpcAnimationControllerComponent& animCtrl,
        AIState actionState, float durationSec, AIState returnState)
    {

        if (animCtrl.actionTimer > 0.0f && ai.CurrentState == actionState)
            return;

        ai.CurrentState = actionState;
       // animCtrl.actionDuration = durationSec;
       ///animCtrl.actionTimer = durationSec;
       // animCtrl.returnState = returnState;
        //animCtrl.request = NpcAnimRequest::Attack;
    }

    // Returns true if still moving, false if finished path
    bool NpcAIMovementSystem::MoveAlongPathXY(NPCAIMovementComponent& ai, Engine::TransformComponent& tr, float dt)
    {
        if (!ai.HasPath())
            return false;

        glm::vec3& npcPos3 = tr.Translation;

        const float reachRadius = 0.10f;
        const float reach2 = reachRadius * reachRadius;

        // consume reached waypoints (prevents dist==0 stuck)
        while (ai.HasPath())
        {
            glm::vec3 wp = ai.Path[ai.PathIndex];
            glm::vec3 d = wp - npcPos3;
            if (glm::dot(d, d) > reach2) break;
            ai.PathIndex++;
        }

        if (!ai.HasPath())
            return false;

        glm::vec3 wp = ai.Path[ai.PathIndex];
        glm::vec3 to = wp - npcPos3;
        float dist2 = glm::dot(to, to);
        if (dist2 <= 1e-8f)
            return true; // next tick will consume

        float dist = std::sqrt(dist2);
        glm::vec3 dir = to / dist;

        float step = ai.MoveSpeed * dt;
        if (step >= dist)
        {
            npcPos3 = wp;
            ai.PathIndex++;
        }
        else
        {
            npcPos3 += dir * step;
        }

        RotateTowardsDirXY(tr, dir, dt);
        return true;
    }



