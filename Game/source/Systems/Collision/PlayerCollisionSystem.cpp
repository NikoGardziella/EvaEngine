#include "PlayerCollisionSystem.h"
#include "Engine/Scene/Scene.h"
#include <Engine/Scene/Components/Player/CharacterControllerComponent.h>
#include <Engine/Debug/Instrumentor.h>

#include <Engine/Scene/Scene.h>
#include <Engine/Scene/Component.h>
#include "Engine/Scene/Entity.h"


void PlayerCollisionSystem::UpdatePlayerCollision(float dt, Engine::Scene* scene)
{
    EE_PROFILE_FUNCTION();

    
    const auto& walls = scene->GetGrid()->GetGridSubcells();

    scene->ForEach<Engine::TransformComponent, CharacterControllerComponent, Engine::CircleCollider2DComponent>(
        [&](Engine::Entity e, Engine::TransformComponent& trsformComp, CharacterControllerComponent& ctrlComp, Engine::CircleCollider2DComponent& cir)
        {
            glm::vec2 p0 = glm::vec2(trsformComp.Translation);
            glm::vec2 delta = ctrlComp.velocity * (ctrlComp.speed * dt);
            float R = cir.Radius;

            glm::vec2 p1 = CollideAndSlideOBBs(walls, p0, delta, R);

            trsformComp.Translation.x = p1.x;
            trsformComp.Translation.y = p1.y;
        });
}

// Sweep a circle vs OBB (expanded by radius). Also handles initial overlap.
PlayerCollisionSystem::SweepHit PlayerCollisionSystem::SweepCircleVsOBB(const Engine::SubCellOBB& obb,
    const glm::vec2& p0, const glm::vec2& delta,  float radius, float skin = 1e-4f)
{
    SweepHit out;

    // Ensure unit frame
    glm::vec2 t = obb.tangent; // already normalized
    glm::vec2 n = perpCCW(t);

    // Transform into OBB local
    glm::vec2 rel0 = p0 - obb.center;
    glm::vec2 s0 = { glm::dot(rel0, t), glm::dot(rel0, n) };
    glm::vec2 v = { glm::dot(delta,  t), glm::dot(delta,  n) };

    // Expanded AABB half extents
    glm::vec2 H = obb.halfExtents + glm::vec2(radius);

    // --- Static overlap: push out along least-penetrating axis
    bool insideX = std::abs(s0.x) <= H.x;
    bool insideY = std::abs(s0.y) <= H.y;
    if (insideX && insideY)
    {
        float ox = H.x - std::abs(s0.x);
        float oy = H.y - std::abs(s0.y);
        glm::vec2 localN(0);
        float push = 0.0f;
        if (ox < oy) { localN = { (s0.x >= 0.f ? 1.f : -1.f), 0.f }; push = ox + skin; }
        else { localN = { 0.f, (s0.y >= 0.f ? 1.f : -1.f) }; push = oy + skin; }

        glm::vec2 worldN = glm::normalize(localN.x * t + localN.y * n);
        out.hit = true;
        out.toi = 0.0f;
        out.normal = worldN;                   // points out of OBB
        out.point = p0 + worldN * push;       // a safe separated position
        return out;
    }

    // --- Dynamic sweep (slabs)
    float tEnter = 0.f, tExit = 1.f;
    int enterAxis = -1; // 0=x, 1=y

    auto sweep1D = [&](float s, float vs, float h, int axis) {
        if (std::abs(vs) < 1e-12f) {
            if (std::abs(s) > h) { tEnter = 1.f; tExit = 0.f; } // miss
            return;
        }
        float inv = 1.f / vs;
        float t1 = (-h - s) * inv;
        float t2 = (h - s) * inv;
        float tin = std::min(t1, t2);
        float tout = std::max(t1, t2);
        if (tin > tEnter) { tEnter = tin; enterAxis = axis; }
        if (tout < tExit) { tExit = tout; }
        };

    sweep1D(s0.x, v.x, H.x, 0);
    sweep1D(s0.y, v.y, H.y, 1);

    if (tEnter > tExit || tExit < 0.f || tEnter > 1.f) return out; // no hit

    out.hit = true;
    out.toi = glm::clamp(tEnter, 0.f, 1.f);

    // Local entry normal opposes motion on that axis
    glm::vec2 localN(0);
    if (enterAxis == 0) localN = { (v.x > 0.f ? -1.f : 1.f), 0.f };
    else                localN = { 0.f, (v.y > 0.f ? -1.f : 1.f) };

    out.normal = glm::normalize(localN.x * t + localN.y * n);

    glm::vec2 hitLocal = s0 + v * out.toi;
    out.point = obb.center + t * hitLocal.x + n * hitLocal.y;
    return out;
}

glm::vec2 PlayerCollisionSystem::CollideAndSlideOBBs( const std::vector<Engine::SubCellOBB>& walls,
    glm::vec2 pos, glm::vec2 delta, float radius)
{

    if (glm::length2(delta) < 1e-12f)
        return pos;

    glm::vec2 rem = delta;
    const float skin = 1e-3f * (radius + 1.f);
    const int maxIters = 4;

    for (int iter = 0; iter < maxIters; ++iter)
    {
        const AABB2 sweptAABB = MakeSweptAABB(pos, rem, radius);

        SweepHit bestDynamic{};
        bool hasStaticPush = false;
        glm::vec2 staticPushPos = pos;
        glm::vec2 staticPushNormal(0.0f);
        float staticPushDist2 = 0.0f;

        for (const auto& obb : walls)
        {
            // Cheap broadphase first
            const AABB2 obbAABB = MakeOBBAABB(obb);
            if (!Overlaps(sweptAABB, obbAABB))
                continue;

            SweepHit h = SweepCircleVsOBB(obb, pos, rem, radius, skin);
            if (!h.hit)
                continue;

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


