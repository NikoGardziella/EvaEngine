#include "NpcAIMovementSystem.h"
#include <Engine/Scene/Components/NPC/NpcAIComponent.h>

#include <Engine/Debug/Instrumentor.h>
#include <Engine/Scene/Scene.h>
#include <Engine/Map/Grid/GridMap.h>
#include <Engine/Core/Core.h>
#include <Engine/Scene/Components/Render/3D/AnimatorComponent.h>
#include <Engine/AssetManager/AssetManager.h>
#include <Engine/Scene/Components/Animation/NpcAnimationControllerComponent.h>
#include <Engine/Scene/Components/NPC/NpcBodyStateComponent.h>

void NpcAIMovementSystem::UpdateNPCAIMovementSystem(float deltatime, Engine::Scene* scene)
{
    EE_PROFILE_FUNCTION();

    Engine::Ref<Engine::GridMap>& grid = scene->GetGrid();

    scene->ForEach<NPCAIMovementComponent, NpcBodyStateComponent, NpcAIStateComponent, Engine::TransformComponent>(
        [&](Engine::Entity entity,
            NPCAIMovementComponent& movementComp,
            NpcBodyStateComponent& bodyStateComp,
            NpcAIStateComponent& npcStateComp,
            Engine::TransformComponent& npcTransformComp)
        {
            auto& physics = scene->GetBox2DPhysicsSystem();

            if (!movementComp.wantsMove)
            {
                movementComp.ClearPath();
                movementComp.repathTimer = 0.0f;
                physics.SetLinearVelocity(entity, { 0.0f, 0.0f });
                return;
            }

            movementComp.movementSpeedMultiplier = 1.0f;
            movementComp.movementSpeedMultiplier *= bodyStateComp.moveSpeedMul;

            switch (npcStateComp.state)
            {
            case AIState::Patrol:
                movementComp.movementSpeedMultiplier = 0.1f;
                break;
            }

            const float speed = movementComp.moveSpeed * movementComp.movementSpeedMultiplier;
            const glm::vec2 npc2(npcTransformComp.Translation.x, npcTransformComp.Translation.y);
            const glm::vec2 goal2 = movementComp.goal2D;

            // ----------------------------------------------------
            // Direct seek mode
            // ----------------------------------------------------
            if (!movementComp.usePath || !grid)
            {
                glm::vec2 to = goal2 - npc2;
                float d2 = glm::dot(to, to);

                const float arriveDist = 0.05f;
                if (d2 <= arriveDist * arriveDist)
                {
                    physics.SetLinearVelocity(entity, { 0.0f, 0.0f });
                    movementComp.ClearPath();
                    movementComp.repathTimer = 0.0f;
                    return;
                }

                if (d2 > 1e-8f)
                {
                    glm::vec2 dir2 = to / glm::sqrt(d2);
                    glm::vec2 vel = dir2 * speed;
                    physics.SetLinearVelocity(entity, vel);

                    if (movementComp.hasFacing)
                        RotateTowardsDirXY(npcTransformComp, movementComp, glm::vec3(movementComp.facing2D.x, movementComp.facing2D.y, 0.0f), deltatime);
                    else
                        RotateTowardsDirXY(npcTransformComp, movementComp, glm::vec3(dir2.x, dir2.y, 0.0f), deltatime);
                }
                else
                {
                    physics.SetLinearVelocity(entity, { 0.0f, 0.0f });
                }

                movementComp.ClearPath();
                movementComp.repathTimer = 0.0f;
                return;
            }

            // ----------------------------------------------------
            // Path mode
            // ----------------------------------------------------
            movementComp.repathTimer += deltatime;

            const float repathStartMoveThresh = 0.25f;
            const bool startMoved = glm::length(npc2 - movementComp.lastRepathStart2D) > repathStartMoveThresh;
            const bool goalChanged = glm::length(goal2 - movementComp.lastRepathGoal2D) > 0.10f;

            const bool needRepath =
                (!movementComp.HasPath()) ||
                (movementComp.repathTimer >= movementComp.repathInterval && (startMoved || goalChanged));

            if (needRepath)
            {
                std::vector<glm::vec2> path2D = grid->FindPathWorld(npc2, goal2);

                movementComp.path.clear();
                movementComp.path.reserve(path2D.size());

                for (const auto& p : path2D)
                {
                    movementComp.path.push_back(glm::vec3(p.x, p.y, npcTransformComp.Translation.z));
                }

                movementComp.pathIndex = 0;
                movementComp.repathTimer = 0.0f;
                movementComp.lastRepathStart2D = npc2;
                movementComp.lastRepathGoal2D = goal2;
            }

            grid->DebugDrawPath(movementComp.path);

            if (!movementComp.HasPath())
            {
                physics.SetLinearVelocity(entity, { 0.0f, 0.0f });
                return;
            }

            if (movementComp.pathIndex >= movementComp.path.size())
            {
                movementComp.ClearPath();
                physics.SetLinearVelocity(entity, { 0.0f, 0.0f });
                return;
            }

            // Skip waypoints already reached
            const float arriveDist = 0.08f;
            while (movementComp.pathIndex < movementComp.path.size())
            {
                glm::vec2 waypoint(
                    movementComp.path[movementComp.pathIndex].x,
                    movementComp.path[movementComp.pathIndex].y
                );

                glm::vec2 toWaypoint = waypoint - npc2;
                float d2 = glm::dot(toWaypoint, toWaypoint);

                if (d2 > arriveDist * arriveDist)
                    break;

                movementComp.pathIndex++;
            }

            if (movementComp.pathIndex >= movementComp.path.size())
            {
                movementComp.ClearPath();
                physics.SetLinearVelocity(entity, { 0.0f, 0.0f });
                return;
            }

            glm::vec2 waypoint(
                movementComp.path[movementComp.pathIndex].x,
                movementComp.path[movementComp.pathIndex].y
            );

            glm::vec2 to = waypoint - npc2;
            float d2 = glm::dot(to, to);

            if (d2 > 1e-8f)
            {
                glm::vec2 dir = to / glm::sqrt(d2);
                glm::vec2 vel = dir * speed;

                physics.SetLinearVelocity(entity, vel);

                if (movementComp.hasFacing)
                    RotateTowardsDirXY(npcTransformComp, movementComp, glm::vec3(movementComp.facing2D.x, movementComp.facing2D.y, 0.0f), deltatime);
                else
                    RotateTowardsDirXY(npcTransformComp, movementComp, glm::vec3(dir.x, dir.y, 0.0f), deltatime);
            }
            else
            {
                physics.SetLinearVelocity(entity, { 0.0f, 0.0f });
            }
        });

}

void NpcAIMovementSystem::RotateTowardsDirXY(Engine::TransformComponent& tr, NPCAIMovementComponent& movementComp,
    const glm::vec3& dir, float dt)
{
    glm::vec2 d(dir.x, dir.y);
    float len2 = glm::dot(d, d);
    if (len2 <= 1e-8f)
        return;

    d /= glm::sqrt(len2);

    const float current = tr.Rotation.z;
    const float target = std::atan2(d.y, d.x) + glm::radians(90.0f);

    float delta = target - current;

    // wrap to [-pi, pi] so it turns the shortest way
    delta = std::atan2(std::sin(delta), std::cos(delta));

    const float maxStep = movementComp.turnSpeed * dt;
    delta = glm::clamp(delta, -maxStep, maxStep);

    tr.Rotation.z = current + delta;
} 