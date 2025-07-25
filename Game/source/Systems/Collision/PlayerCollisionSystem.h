#pragma once
#include "entt.hpp"
#include "Engine.h"
#include "Engine/Scene/Components/Player/CharacterControllerComponent.h"

class PlayerCollisionSystem
{
    struct RaycastHit
    {
        float t;
        glm::vec2 normal;
        bool hit = false;
    };

    struct SweepResult
    {
        bool hit = false;
        float timeOfImpact = 1.0f;
        glm::vec2 hitPoint;
        glm::vec2 normal;
    };

public:
	static void UpdatePlayerCollision(entt::registry& registry, float deltaTime, Engine::Scene* scene);
 
    //static void ProcessPlayerMovement(Engine::TransformComponent& playerTransform, CharacterControllerComponent& controller, const Engine::CircleCollider2DComponent& playerCollider, const Engine::IDComponent& playerIDComp, const StaticView staticView, float deltaTime);





private:

    static RaycastHit SweptCircleAABB(glm::vec2 circleCenter, float radius, glm::vec2 velocity,
        glm::vec2 aabbMin, glm::vec2 aabbMax);
    
  
 
};

