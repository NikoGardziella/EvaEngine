#include "PlayerFloorSystem.h"
#include <Engine/Scene/Components/Player/CharacterControllerComponent.h>
#include <Engine/Scene/Components/Map/FloorComponent.h>
#include <Engine/Scene/Components/Map/StairsComponent.h>

void PlayerFloorSystem::UpdatePlayerFloorSystem(float dt, Engine::Scene* scene)
{
    EE_PROFILE_FUNCTION();

    if (!scene)
        return;

    scene->ForEach<Engine::TransformComponent, CharacterControllerComponent, Engine::CircleCollider2DComponent, FloorComponent>(
            [&](Engine::Entity playerEntity, Engine::TransformComponent& playerTransform, CharacterControllerComponent& ctrlComp,
                Engine::CircleCollider2DComponent& circle, FloorComponent& floorComp)
            {
                glm::vec2 playerPos = glm::vec2(playerTransform.Translation);

                if (!floorComp.IsChangingFloor)
                {
                    scene->ForEach<Engine::TransformComponent, StairsComponent>([&](Engine::Entity stairsEntity, Engine::TransformComponent& stairsTransform,
                                StairsComponent& stairs)
                            {
                                if (floorComp.IsChangingFloor)
                                    return;

                                if (floorComp.Floor != stairs.FromFloor &&
                                    floorComp.Floor != stairs.ToFloor)
                                {
                                    return;
                                }

                                glm::vec2 stairsPos = glm::vec2(stairsTransform.Translation);
                                float dist = glm::distance(playerPos, stairsPos);

                                if (dist > stairs.TriggerRadius + circle.Radius)
                                    return;

                                glm::vec2 moveDir = glm::vec2(0.0f);

                                if (glm::length(ctrlComp.velocity) > 0.001f)
                                    moveDir = glm::normalize(ctrlComp.velocity);

                                glm::vec2 entryDir = glm::normalize(stairs.EntryDir);

                                float dirDot = glm::dot(moveDir, entryDir);

                                // Going up
                                if (floorComp.Floor == stairs.FromFloor && dirDot > 0.5f)
                                {
                                    floorComp.TargetFloor = stairs.ToFloor;
                                    floorComp.FloorT = 0.0f;
                                    floorComp.IsChangingFloor = true;
                                }

                                // Going down
                                if (floorComp.Floor == stairs.ToFloor && dirDot < -0.5f)
                                {
                                    floorComp.TargetFloor = stairs.FromFloor;
                                    floorComp.FloorT = 0.0f;
                                    floorComp.IsChangingFloor = true;
                                }
                            });
                }

                if (floorComp.IsChangingFloor)
                {
                    floorComp.FloorT += dt * floorComp.ClimbSpeed;

                    if (floorComp.FloorT >= 1.0f)
                    {
                        floorComp.Floor = floorComp.TargetFloor;
                        floorComp.FloorT = 0.0f;
                        floorComp.IsChangingFloor = false;
                    }
                }
            });
}