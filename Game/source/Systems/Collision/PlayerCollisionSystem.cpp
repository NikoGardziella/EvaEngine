#include "PlayerCollisionSystem.h"
#include "Engine/Scene/Scene.h"
#include <Engine/Scene/Components/Player/CharacterControllerComponent.h>
#include <Engine/Debug/Instrumentor.h>

#include <Engine/Scene/Scene.h>
#include <Engine/Scene/Component.h>
#include "Engine/Scene/Entity.h"
#include <Engine/Scene/Components/Map/FloorComponent.h>






void PlayerCollisionSystem::UpdatePlayerCollision(float dt, Engine::Scene* scene)
{
    EE_PROFILE_FUNCTION();

    const auto& walls = scene->GetGrid()->GetGridSubcells();

    scene->ForEach<Engine::TransformComponent, CharacterControllerComponent, Engine::CircleCollider2DComponent,
        FloorComponent>([&](Engine::Entity e, Engine::TransformComponent& transformComp, CharacterControllerComponent& ctrlComp,
                Engine::CircleCollider2DComponent& cir, FloorComponent& floorComp)
            {
                glm::vec2 delta = ctrlComp.velocity * (ctrlComp.speed * dt);
                float R = cir.Radius;

                glm::vec2 p0 = glm::vec2(transformComp.Translation);
                glm::vec2 p1 = CollideAndSlideOBBs(walls, p0, delta, R, floorComp);


                transformComp.Translation.x = p1.x;
                transformComp.Translation.y = p1.y;

            });
}

glm::vec2 PlayerCollisionSystem::CollideAndSlideOBBs( const std::vector<Engine::SubCellOBB>& walls,
    glm::vec2 pos, glm::vec2 delta, float radius, const FloorComponent& floorComp)
{
    if (glm::length2(delta) < 1e-12f)
        return pos;

    glm::vec2 rem = delta;
    const float skin = 1e-3f * (radius + 1.f);
    const int maxIters = 4;

    for (int iter = 0; iter < maxIters; ++iter)
    {
        const CollisionSystemUtils::AABB2 sweptAABB = CollisionSystemUtils::MakeSweptAABB(pos, rem, radius);

        CollisionSystemUtils::SweepHit bestDynamic{};
        bool hasStaticPush = false;
        glm::vec2 staticPushPos = pos;
        glm::vec2 staticPushNormal(0.0f);
        float staticPushDist2 = 0.0f;

        for (const auto& obb : walls)
        {
            if (floorComp.IsChangingFloor)
            {
                if (obb.Type != Engine::eSubCellType::StairRail)
                {
                    continue;
                }
            }
            else
            {

                if (obb.Floor != floorComp.Floor)
                {

                    continue;
                }
            }
            // Cheap broadphase first
            const CollisionSystemUtils::AABB2 obbAABB = CollisionSystemUtils::MakeOBBAABB(obb);
            if (!CollisionSystemUtils::Overlaps(sweptAABB, obbAABB))
            {
                continue;
            }

            CollisionSystemUtils::SweepHit h = CollisionSystemUtils::SweepCircleVsOBB(obb, pos, rem, radius, skin);
            if (!h.hit)
            {
                continue;
            }

            // Static overlap candidate: keep only the strongest push
            if (h.toi == 0.0f && glm::length2(h.normal) > 0.0f)
            {
                glm::vec2 pushVec = h.point - pos;
                float d2 = glm::length2(pushVec);

                if (!hasStaticPush || d2 > staticPushDist2)
                {
                    hasStaticPush = true;
                    staticPushPos = h.point;
                    staticPushNormal = h.normal;
                    staticPushDist2 = d2;
                }
                continue;
            }

            // Earliest dynamic hit wins
            if (!bestDynamic.hit || h.toi < bestDynamic.toi)
                bestDynamic = h;
        }

        // Prefer dynamic collision if found
        if (bestDynamic.hit)
        {
            float tMove = std::max(0.0f, bestDynamic.toi - 1e-4f);
            pos += rem * tMove;

            glm::vec2 leftover = rem * (1.0f - tMove);

            float vn = glm::dot(leftover, bestDynamic.normal);
            if (vn < 0.0f)
                leftover -= bestDynamic.normal * vn;

            pos += bestDynamic.normal * skin;

            if (glm::length2(leftover) < 1e-10f)
                break;

            rem = leftover;
            continue;
        }

        // No dynamic hit, but overlapping: resolve once
        if (hasStaticPush)
        {
            pos = staticPushPos;

            float vn = glm::dot(rem, staticPushNormal);
            if (vn < 0.0f)
                rem -= staticPushNormal * vn;

            if (glm::length2(rem) < 1e-10f)
                break;

            continue;
        }

        // No collision at all
        pos += rem;
        break;
    }

    return pos;
}


