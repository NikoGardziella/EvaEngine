#include "NPCAIVisionSystem.h"
#include "Engine/Scene/Components/NPC/NpcAIComponent.h"
#include "Engine/Scene/Component.h"

#include <glm/glm.hpp>
#include <Engine/Scene/Components/Player/CharacterControllerComponent.h>
#include <Engine/Debug/Instrumentor.h>
#include <Engine/Scene/Scene.h>
#include "NpcAIMovementSystem.h"
#include <Engine/Map/Grid/GridMap.h>
#include <Engine/Scene/Entity.h>

void NPCAIVisionSystem::UpdateNPCAIVisionSystem(float dt, Engine::Scene* scene)
{
    EE_PROFILE_FUNCTION();

    Engine::Ref<Engine::GridMap>& grid = scene->GetGrid();

    scene->ForEach<NPCAIVisionComponent, Engine::TransformComponent>(
        [&](Engine::Entity npcEntity, NPCAIVisionComponent& npcVisionComp, Engine::TransformComponent& npcTransformComp)
        {
            // ---- Advance timers ----
            npcVisionComp.timeSinceSeen += dt;
            npcVisionComp.losCheckTimer -= dt;

            // We do NOT reset everything here.
            // We will only clear/overwrite if we fail/succeed to acquire.

            // NPC forward direction (top-down, facing by Rotation.z)
            const float npcYaw = npcTransformComp.Rotation.z;
            glm::vec2 npcForward2(std::cos(npcYaw), std::sin(npcYaw));
            const float fwdLen2 = glm::dot(npcForward2, npcForward2);
            if (fwdLen2 > 1e-8f) npcForward2 /= std::sqrt(fwdLen2);

            const float viewRadiusSq = npcVisionComp.ViewRadius * npcVisionComp.ViewRadius;
            const bool  limitByAngle = (npcVisionComp.ViewAngle < 360.0f);
            const float halfFov = 0.5f * npcVisionComp.ViewAngle;

            bool acquiredThisTick = false;

            // We usually have only one player. If multiple, this picks the first that passes checks.
            scene->ForEach<CharacterControllerComponent, Engine::TransformComponent>(
                [&](Engine::Entity playerEntity, CharacterControllerComponent& /*playerCtrl*/,
                    Engine::TransformComponent& playerTr)
                {
                    if (acquiredThisTick) return;

                    const glm::vec3 toPlayer3 = playerTr.Translation - npcTransformComp.Translation;
                    const glm::vec2 toPlayer2(toPlayer3.x, toPlayer3.y);


                    const float distSq = glm::dot(toPlayer2, toPlayer2);
                    npcVisionComp.distToTarget = std::sqrt(distSq);

                    if (distSq > viewRadiusSq) return;

                    if (limitByAngle)
                    {
                        const float len2 = glm::dot(toPlayer2, toPlayer2);
                        if (len2 > 1e-8f)
                        {
                            const glm::vec2 toDir = toPlayer2 / std::sqrt(len2);
                            const float cosTheta = glm::clamp(glm::dot(npcForward2, toDir), -1.0f, 1.0f);
                            const float angleDeg = glm::degrees(std::acos(cosTheta));
                            if (angleDeg > halfFov) return;
                        }
                    }

                    // ---- LOS handling ----
                    // We only update vision.hasLOS when we actually evaluate LOS (timer elapsed),
                    // or when we are *certain* we lost the target.
                    bool hasLosNow = true;

                    if (grid)
                    {
                        if (npcVisionComp.losCheckTimer <= 0.0f)
                        {
                            hasLosNow = HasLOSNow(grid, npcTransformComp.Translation, playerTr.Translation, npcVisionComp.debugDraw);
                            npcVisionComp.losCheckTimer = npcVisionComp.losCheckInterval;

                            // commit result
                            npcVisionComp.hasLOS = hasLosNow ? 1 : 0;
                        }
                        else
                        {
                            // Throttled: don't raycast. Only allow "carry" if it's the same target we last saw.
                            // This avoids snapping to random targets while throttled.
                            if (npcVisionComp.lastSeenTarget == playerEntity && npcVisionComp.hasLOS)
                                hasLosNow = true;
                            else
                                hasLosNow = false;
                        }
                    }

                    if (!hasLosNow)
                        return;

                    // ---- Acquire target (commit) ----
                    npcVisionComp.VisibleTarget = playerEntity;

                    npcVisionComp.lastSeenTarget = playerEntity;
                    npcVisionComp.lastSeenPos = playerTr.Translation;
                    npcVisionComp.timeSinceSeen = 0.0f;

                    acquiredThisTick = true;
                });

            // If we didn't acquire a visible target this tick, clear VisibleTarget.
            // But do NOT wipe lastSeenPos/lastSeenTarget/timeSinceSeen (that's the whole point of caching).
            if (!acquiredThisTick)
            {
                npcVisionComp.VisibleTarget = Engine::Entity{}; // or entt::null, depending on your type
                npcVisionComp.distToTarget = 0.0f;

                // If we *expected* to see someone and didn't, you can optionally mark hasLOS = 0.
                // I prefer keeping hasLOS as "last evaluated raycast result" (only changes when we raycast).
                // If you want "hasLOS means visible right now", uncomment this:
                // vision.hasLOS = 0;
            }
        });
}


bool NPCAIVisionSystem::HasLOSNow(const Engine::Ref<Engine::GridMap>& grid, const glm::vec3& npcPos3, const glm::vec3& targetPos3, bool debugDraw)
{
    glm::vec2 npc2(npcPos3.x, npcPos3.y);
    glm::vec2 tgt2(targetPos3.x, targetPos3.y);
    return grid->HasLineOfSight(npc2, tgt2, debugDraw);
}
