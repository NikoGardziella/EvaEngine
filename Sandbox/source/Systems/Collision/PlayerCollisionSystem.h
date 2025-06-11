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

public:
	static void UpdatePlayerCollision(entt::registry& registry, float deltaTime, Engine::Scene* scene);
    //static void ProcessPlayerMovement(Engine::TransformComponent& playerTransform, CharacterControllerComponent& controller, const Engine::CircleCollider2DComponent& playerCollider, const Engine::IDComponent& playerIDComp, const StaticView staticView, float deltaTime);

    static void HandleStaticCollision(Engine::TransformComponent& playerTransform, CharacterControllerComponent& controller, const glm::vec2& startPos, const glm::vec2& attemptedMove, const std::function<bool(const glm::vec2&)>& IsCollidingWithStatic);

    static void HandlePixelCollision(Engine::TransformComponent& playerTransform, CharacterControllerComponent& controller, const Engine::IDComponent& playerIDComp, const glm::vec2& targetPos, const glm::vec2& offset, float radius);

    static void ApplyCollisionCorrection(Engine::TransformComponent& playerTransform, CharacterControllerComponent& controller, const glm::vec2& targetPos, const glm::vec2& totalPush, int validCollisions);


private:
    static bool CollidesWithAnyPixel(glm::vec2 center, float radius);
    static RaycastHit SweptCircleAABB(glm::vec2 circleCenter, float radius, glm::vec2 velocity,
        glm::vec2 aabbMin, glm::vec2 aabbMax);
    
    template<typename StaticView>
    static void ProcessPlayerMovement(Engine::TransformComponent& playerTransform, CharacterControllerComponent& controller, const Engine::CircleCollider2DComponent& playerCollider, const Engine::IDComponent& playerIDComp, const StaticView& staticView, float deltaTime);

    template<typename StaticView>
    static void HandleStaticCollision(Engine::TransformComponent& playerTransform, CharacterControllerComponent& controller, const glm::vec2& startPos, const glm::vec2& attemptedMove, const StaticView& staticView, const glm::vec2& offset, float radius);

    
};

