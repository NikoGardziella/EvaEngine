#pragma once
#include "entt.hpp"
#include "Engine.h"


class PlayerCollisionSystem
{
    struct RaycastHit
    {
        float t;
        glm::vec2 normal;
        bool hit = false;
    };

public:
	static void UpdatePlayerCollision(entt::registry& registry, float deltaTime, Engine::Scene* scene);


private:
    static RaycastHit SweptCircleAABB(glm::vec2 circleCenter, float radius, glm::vec2 velocity,
        glm::vec2 aabbMin, glm::vec2 aabbMax);
    
};

