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

void NpcAIMovementSystem::UpdateNPCAIMovementSystem(float deltatime, Engine::Scene* scene)
{
    EE_PROFILE_FUNCTION();

    Engine::Ref<Engine::GridMap>& grid = scene->GetGrid();
    

    
    
    // Execute movement orders (movement only)
    scene->ForEach<NPCAIMovementComponent, Engine::TransformComponent>(
        [&](Engine::Entity, NPCAIMovementComponent& movementComp, Engine::TransformComponent& npcTransformComp)
        {
            if (!movementComp.wantsMove)
            {
                // prevent stale path resumes
                movementComp.ClearPath();
                movementComp.repathTimer = 0.0f;
                return;
            }

            const glm::vec2 npc2(npcTransformComp.Translation.x, npcTransformComp.Translation.y);
            const glm::vec2 goal2 = movementComp.goal2D;

            auto FaceDir2D = [&](const glm::vec2& dir2)
                {
                    glm::vec2 d = dir2;
                    float len2 = glm::dot(d, d);
                    if (len2 <= 1e-8f) return;

                    d /= glm::sqrt(len2);

                    if (movementComp.hasFacing)
                        RotateTowardsDirXY(npcTransformComp, glm::vec3(movementComp.facing2D.x, movementComp.facing2D.y, 0.0f), deltatime);
                    else
                        RotateTowardsDirXY(npcTransformComp, glm::vec3(d.x, d.y, 0.0f), deltatime);
                };

            // ---- Direct seek (no path) ----
            if (!movementComp.usePath || !grid)
            {
                glm::vec2 to = goal2 - npc2;
                float d2 = glm::dot(to, to);
                if (d2 > 1e-8f)
                {
                    glm::vec2 dir2 = to / glm::sqrt(d2);

                    npcTransformComp.Translation.x += dir2.x * movementComp.moveSpeed * deltatime;
                    npcTransformComp.Translation.y += dir2.y * movementComp.moveSpeed * deltatime;

                    FaceDir2D(dir2);
                }

                movementComp.ClearPath();
                movementComp.repathTimer = 0.0f;
                return;
            }

            // ---- Path mode ----
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

            // debug
            grid->DebugDrawPath(movementComp.path);

            if (!movementComp.HasPath())
                return;

            const bool moving = MoveAlongPathXY(movementComp, npcTransformComp, deltatime);

           

            if (!moving)
            {
                // reached end or got stuck
                movementComp.ClearPath();
            }
        });


    
    struct AgentRef
    {
        Engine::TransformComponent* tr = nullptr;
        glm::vec2 pos2 = { 0.0f, 0.0f };
        float radius = 0.5f;
    };

    std::vector<AgentRef> agents;
    agents.reserve(128);

    scene->ForEach<Engine::TransformComponent, NPCAIMovementComponent>(
        [&](Engine::Entity, Engine::TransformComponent& npcTransformComp, NPCAIMovementComponent& movementComp)
        {
            agents.push_back({ &npcTransformComp, { npcTransformComp.Translation.x, npcTransformComp.Translation.y }, movementComp.radius });
        });

    const size_t count = agents.size();
    if (count < 2)
        return; // <-- safe now; movement already executed

    std::vector<glm::vec2> push(count, glm::vec2(0.0f));

    for (size_t i = 0; i < count; ++i)
    {
        const glm::vec2 pi = agents[i].pos2;
        const float ri = agents[i].radius;

        for (size_t j = i + 1; j < count; ++j)
        {
            const glm::vec2 pj = agents[j].pos2;
            const float rj = agents[j].radius;

            glm::vec2 diff = pi - pj;
            float dist2 = glm::dot(diff, diff);

            const float minDist = ri + rj;
            const float minDist2 = minDist * minDist;

            if (dist2 < minDist2 && dist2 > 1e-8f)
            {
                float dist = glm::sqrt(dist2);
                glm::vec2 dir = diff / dist;

                float pen = (minDist - dist);

                glm::vec2 d = dir * (pen * 0.5f);
                push[i] += d;
                push[j] -= d;
            }
        }
    }

    const float sepStrength = 12.0f;
    const float maxPushPerFrame = 0.80f;

    for (size_t i = 0; i < count; ++i)
    {
        glm::vec2 d = push[i] * (sepStrength * deltatime);

        
        d *= 0.35f;

        float len2 = glm::dot(d, d);
        if (len2 > maxPushPerFrame * maxPushPerFrame)
        {
            float len = glm::sqrt(len2);
            d *= (maxPushPerFrame / len);
        }

        agents[i].tr->Translation.x += d.x;
        agents[i].tr->Translation.y += d.y;
    }

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


    // Returns true if still moving, false if finished path
    bool NpcAIMovementSystem::MoveAlongPathXY(NPCAIMovementComponent& movementComp, Engine::TransformComponent& npcTransformComp, float deltatime)
    {
        if (!movementComp.HasPath())
            return false;

        glm::vec3& pos3 = npcTransformComp.Translation;

        const float stepTotal = movementComp.moveSpeed * deltatime;
        if (stepTotal <= 1e-6f)
            return true;

        if (movementComp.pathIndex + 1u >= (uint32_t)movementComp.path.size())
        {
            const glm::vec3& wp = movementComp.path[movementComp.pathIndex];
            glm::vec2 to(wp.x - pos3.x, wp.y - pos3.y);
            float d2 = glm::dot(to, to);
            if (d2 <= 1e-6f)
            {
                movementComp.pathIndex++;
                return movementComp.HasPath();
            }

            float d = std::sqrt(d2);
            glm::vec2 dir = to / d;

            float step = stepTotal;
            if (step >= d)
            {
                pos3.x = wp.x;
                pos3.y = wp.y;
                movementComp.pathIndex++;
            }
            else
            {
                pos3.x += dir.x * step;
                pos3.y += dir.y * step;
            }

            RotateTowardsDirXY(npcTransformComp, glm::vec3(dir.x, dir.y, 0.0f), deltatime);
            return true;
        }

        auto Dot2 = [](const glm::vec2& a, const glm::vec2& b) -> float
            {
                return a.x * b.x + a.y * b.y; 
            };

        auto Len2 = [&](const glm::vec2& v) -> float
            {
                return Dot2(v, v);
            };

        float remaining = stepTotal;

        glm::vec2 movedDir(0.0f, 0.0f);
        bool moved = false;

        // Consume movement over multiple segments if needed
        while (remaining > 1e-6f)
        {
            if (movementComp.pathIndex + 1u >= (uint32_t)movementComp.path.size())
                break;

            const glm::vec2 A(movementComp.path[movementComp.pathIndex].x, movementComp.path[movementComp.pathIndex].y);
            const glm::vec2 B(movementComp.path[movementComp.pathIndex + 1u].x, movementComp.path[movementComp.pathIndex + 1u].y);

            glm::vec2 P(pos3.x, pos3.y);
            glm::vec2 AB = B - A;
            float ab2 = Len2(AB);

            // Degenerate segment: skip it
            if (ab2 <= 1e-8f)
            {
                movementComp.pathIndex++;
                continue;
            }

            // Project P onto segment to get a stable "progress" even if we got pushed sideways
            float t = Dot2(P - A, AB) / ab2;
            t = glm::clamp(t, 0.0f, 1.0f);

            glm::vec2 proj = A + AB * t;

            // If we're far off the segment (separation can do this), gently pull toward projection first.
            // This prevents orbiting around corners.
            glm::vec2 toProj = proj - P;
            float toProjLen2 = Len2(toProj);

            // Small correction cap so it doesn't "snap" sideways too hard
            const float correctionMax = remaining * 0.35f;
            if (toProjLen2 > 1e-8f)
            {
                float toProjLen = std::sqrt(toProjLen2);
                float corr = std::min(correctionMax, toProjLen);
                glm::vec2 corrDir = toProj / toProjLen;

                // apply correction
                pos3.x += corrDir.x * corr;
                pos3.y += corrDir.y * corr;

                movedDir = corrDir;
                moved = true;
                remaining -= corr;

                // recompute P after correction
                P = glm::vec2(pos3.x, pos3.y);
                // continue to forward movement in same segment
            }

            // Recompute projection after correction
            t = Dot2(P - A, AB) / ab2;
            t = glm::clamp(t, 0.0f, 1.0f);

            // How much distance remains on this segment from current t to the end
            float abLen = std::sqrt(ab2);
            float distToEnd = (1.0f - t) * abLen;

            // Forward direction along the segment
            glm::vec2 segDir = AB / abLen;

            if (remaining >= distToEnd - 1e-6f)
            {
                // We can reach the end of this segment this tick: snap to B and advance
                pos3.x = B.x;
                pos3.y = B.y;

                movedDir = segDir;
                moved = true;

                remaining -= distToEnd;
                movementComp.pathIndex++;

                // If we advanced to the last point, we're done
                if (movementComp.pathIndex + 1u >= (uint32_t)movementComp.path.size())
                    break;
            }
            else
            {
                // Move forward along the segment
                pos3.x += segDir.x * remaining;
                pos3.y += segDir.y * remaining;

                movedDir = segDir;
                moved = true;

                remaining = 0.0f;
            }
        }

        // Rotate based on the last meaningful motion direction
        if (moved)
        {
            RotateTowardsDirXY(npcTransformComp, glm::vec3(movedDir.x, movedDir.y, 0.0f), deltatime);
        }

        // If we've consumed all segments, mark finished
        if (movementComp.pathIndex >= (uint32_t)movementComp.path.size() - 1u)
        {
            // Keep index valid; movement system can ClearPath when this returns false if desired
            return false;
        }

        return true;
    }




