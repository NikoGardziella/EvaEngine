#pragma once
#include "glm/glm.hpp"
#include <cstdlib>


class CollisionSystemUtils {

public:
    struct AABB2
    {
        glm::vec2 min;
        glm::vec2 max;
    };

    struct CollisionMoveResult
    {
        glm::vec2 FinalPosition{};
        bool Hit = false;
        glm::vec2 HitPoint{};
        glm::vec2 HitNormal{};

        std::vector<uint64_t> HitSubCellKeys;
    };

    struct SweepHit {
        bool hit = false;
        float toi = 1.0f;        // time of impact in [0,1]
        glm::vec2 normal{ 0 };     // world-space unit normal at hit (points out of OBB)
        glm::vec2 point{ 0 };      // world-space hit point (contact)
    };


public:

    static inline  glm::vec2 AbsVec2(const glm::vec2& v)
    {
        return glm::vec2(std::abs(v.x), std::abs(v.y));
    }

    static inline AABB2 MakeSweptAABB(const glm::vec2& p0, const glm::vec2& delta, float radius)
    {
        glm::vec2 p1 = p0 + delta;
        glm::vec2 mn = glm::min(p0, p1) - glm::vec2(radius);
        glm::vec2 mx = glm::max(p0, p1) + glm::vec2(radius);
        return { mn, mx };
    }

    static inline AABB2 MakeOBBAABB(const Engine::SubCellOBB& obb)
    {
        glm::vec2 t = glm::normalize(obb.tangent);
        glm::vec2 n = glm::vec2(-t.y, t.x);

        glm::vec2 ex = AbsVec2(t) * obb.halfExtents.x;
        glm::vec2 ey = AbsVec2(n) * obb.halfExtents.y;
        glm::vec2 e = ex + ey;

        return { obb.center - e, obb.center + e };
    }

  
    static inline bool Overlaps(const AABB2& a, const AABB2& b)
    {
        if (a.max.x < b.min.x || a.min.x > b.max.x) return false;
        if (a.max.y < b.min.y || a.min.y > b.max.y) return false;
        return true;
    }

    inline static  glm::vec2 perpCCW(const glm::vec2& v) { return glm::vec2(-v.y, v.x); }

    // Sweep a circle vs OBB (expanded by radius). Also handles initial overlap.
    static SweepHit SweepCircleVsOBB(const Engine::SubCellOBB& obb,
        const glm::vec2& p0, const glm::vec2& delta, float radius, float skin = 1e-4f)
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
};