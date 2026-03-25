#pragma once

#include <Engine/Map/Grid/GridMap.h>
#include "Engine/Map/Grid/GridUtils/GridUtils.h"
#include <Engine/Scene/Scene.h>


class Scene;
class PlayerCollisionSystem
{
private:
    struct SweepHit {
        bool hit = false;
        float toi = 1.0f;        // time of impact in [0,1]
        glm::vec2 normal{ 0 };     // world-space unit normal at hit (points out of OBB)
        glm::vec2 point{ 0 };      // world-space hit point (contact)
    };
    
    struct AABB2
    {
        glm::vec2 min;
        glm::vec2 max;
    };


   
public:
    static void UpdatePlayerCollision(float deltaTime, Engine::Scene* scene);
private:
    static PlayerCollisionSystem::SweepHit SweepCircleVsOBB(const Engine::SubCellOBB& obb, const glm::vec2& p0, const glm::vec2& delta, float radius, float skin);

    static glm::vec2 CollideAndSlideOBBs(const std::vector<Engine::SubCellOBB>& walls, glm::vec2 pos, glm::vec2 delta, float radius);
    inline static  glm::vec2 perpCCW(const glm::vec2& v) { return glm::vec2(-v.y, v.x); }

 

    static inline AABB2 MakeSweptAABB(const glm::vec2& p0, const glm::vec2& delta, float radius)
    {
        glm::vec2 p1 = p0 + delta;
        glm::vec2 mn = glm::min(p0, p1) - glm::vec2(radius);
        glm::vec2 mx = glm::max(p0, p1) + glm::vec2(radius);
        return { mn, mx };
    }

    inline static  glm::vec2 AbsVec2(const glm::vec2& v)
    {
        return glm::vec2(std::abs(v.x), std::abs(v.y));
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
   
};
