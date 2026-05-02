#include "PlayerFloorSystem.h"
#include <Engine/Scene/Components/Player/CharacterControllerComponent.h>
#include <Engine/Scene/Components/Map/FloorComponent.h>
#include <Engine/Scene/Components/Map/StairsComponent.h>
#include <Engine/Map/Utils/IsoTileUtils.h>
static glm::vec2 ClosestPointOnSegment(
    const glm::vec2& p,
    const glm::vec2& a,
    const glm::vec2& b)
{
    glm::vec2 ab = b - a;
    float abLenSq = glm::dot(ab, ab);

    if (abLenSq <= 0.00001f)
        return a;

    float t = glm::dot(p - a, ab) / abLenSq;
    t = glm::clamp(t, 0.0f, 1.0f);

    return a + ab * t;
}

static float GetPointTOnSegment(
    const glm::vec2& p,
    const glm::vec2& a,
    const glm::vec2& b)
{
    glm::vec2 ab = b - a;
    float abLenSq = glm::dot(ab, ab);

    if (abLenSq <= 0.00001f)
        return 0.0f;

    float t = glm::dot(p - a, ab) / abLenSq;
    return glm::clamp(t, 0.0f, 1.0f);
}
void PlayerFloorSystem::UpdatePlayerFloorSystem(float dt, Engine::Scene* scene)
{
    EE_PROFILE_FUNCTION();

    if (!scene)
        return;

    scene->ForEach<
        Engine::TransformComponent,
        CharacterControllerComponent,
        Engine::CircleCollider2DComponent,
        FloorComponent>(
            [&](Engine::Entity playerEntity,
                Engine::TransformComponent& playerTransform,
                CharacterControllerComponent& ctrlComp,
                Engine::CircleCollider2DComponent& circle,
                FloorComponent& floorComp)
            {
                glm::vec2 playerPos = glm::vec2(playerTransform.Translation);

                glm::vec2 moveDir(0.0f);
                float moveLen = glm::length(ctrlComp.velocity);

                if (moveLen > 0.001f)
                    moveDir = glm::normalize(ctrlComp.velocity);

                bool onStairs = false;
                float stairDirAmount = 0.0f;
                float stairT = 0.0f;

                int16_t stairFromFloor = 0;
                int16_t stairToFloor = 0;

                scene->ForEach<Engine::TransformComponent, Engine::TileComponent>(
                    [&](Engine::Entity tileEntity,
                        Engine::TransformComponent& tileTransform,
                        Engine::TileComponent& tileComp)
                    {
                        if (onStairs)
                            return;

                        glm::vec2 entityOrigin = glm::vec2(tileTransform.Translation);

                        for (const Engine::StairLink& stairs : tileComp.stairLinks)
                        {
                            if (onStairs)
                                return;

                            if (floorComp.Floor != stairs.FromFloor &&
                                floorComp.Floor != stairs.ToFloor)
                            {
                                continue;
                            }

                            glm::vec2 bottomPoint = entityOrigin + stairs.BottomLocal;
                            glm::vec2 topPoint = entityOrigin + stairs.TopLocal;

                            glm::vec2 stairVector = topPoint - bottomPoint;

                            if (glm::length(stairVector) <= 0.0001f)
                                continue;

                            glm::vec2 closestPoint = ClosestPointOnSegment(
                                playerPos,
                                bottomPoint,
                                topPoint);

                            float dist = glm::distance(playerPos, closestPoint);

                            if (dist > stairs.TriggerRadius + circle.Radius)
                                continue;

                            glm::vec2 stairDir = glm::normalize(stairVector);

                            float dirDot = 0.0f;
                            if (moveLen > 0.001f)
                                dirDot = glm::dot(moveDir, stairDir);

                            onStairs = true;
                            stairDirAmount = dirDot;
                            stairT = GetPointTOnSegment(playerPos, bottomPoint, topPoint);

                            stairFromFloor = stairs.FromFloor;
                            stairToFloor = stairs.ToFloor;

                            Engine::VulkanRenderer2D::DrawLine(
                                glm::vec3(bottomPoint.x, bottomPoint.y, 0.0f),
                                glm::vec3(topPoint.x, topPoint.y, 0.0f),
                                glm::vec4(1.0f, 0.2f, 0.2f, 1.0f));
                        }
                    });

                if (!onStairs)
                {
                    floorComp.IsChangingFloor = false;
                    return;
                }

                if (moveLen <= 0.001f)
                {
                    floorComp.IsChangingFloor = false;
                    return;
                }

                constexpr float DirectionThreshold = 0.2f;
                constexpr float CompleteThreshold = 0.95f;

                if (floorComp.Floor == stairFromFloor && stairDirAmount > DirectionThreshold)
                {
                    floorComp.TargetFloor = stairToFloor;
                    floorComp.IsChangingFloor = true;
                    floorComp.FloorT = stairT;
                }
                else if (floorComp.Floor == stairToFloor && stairDirAmount < -DirectionThreshold)
                {
                    floorComp.TargetFloor = stairFromFloor;
                    floorComp.IsChangingFloor = true;
                    floorComp.FloorT = 1.0f - stairT;
                }
                else
                {
                    floorComp.IsChangingFloor = false;
                    return;
                }

                floorComp.FloorT = glm::clamp(floorComp.FloorT, 0.0f, 1.0f);

                if (floorComp.FloorT >= CompleteThreshold)
                {
                    floorComp.Floor = floorComp.TargetFloor;
                    floorComp.TargetFloor = floorComp.Floor;
                    floorComp.FloorT = 0.0f;
                    floorComp.IsChangingFloor = false;
                }
            });
}