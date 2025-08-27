#pragma once
#include <algorithm>   // std::clamp, std::max
#include <cmath>       // std::floor
#include <cstdint>
#include "entt.hpp"
#include "Engine.h"
#include <Engine/Map/Grid/GridMap.h>

class PlayerCollisionSystem
{
private:
    struct SweepHit {
        bool hit = false;
        float toi = 1.0f;        // time of impact in [0,1]
        glm::vec2 normal{ 0 };     // world-space unit normal at hit (points out of OBB)
        glm::vec2 point{ 0 };      // world-space hit point (contact)
    };
   
   
public:
    static void UpdatePlayerCollision(entt::registry& registry, float deltaTime, Engine::Scene* scene);
private:
    static PlayerCollisionSystem::SweepHit SweepCircleVsOBB(const Engine::SubCellOBB& obb, const glm::vec2& p0, const glm::vec2& delta, float radius, float skin);

    static glm::vec2 CollideAndSlideOBBs(const std::vector<Engine::SubCellOBB>& walls, glm::vec2 pos, glm::vec2 delta, float radius);
    inline static  glm::vec2 perpCCW(const glm::vec2& v) { return glm::vec2(-v.y, v.x); }

 

   
};
