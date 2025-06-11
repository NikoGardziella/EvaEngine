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
    static glm::vec2 SmoothSlideMovement(const glm::vec2& startPos, const glm::vec2& desiredMove, const glm::vec2& offset, float radius, uint32_t playerID);
    static glm::vec2 TrySlideMove(const glm::vec2& startPos, const glm::vec2& moveVector, const glm::vec2& offset, float radius, uint32_t playerID);
    static glm::vec2 AttemptSlide(const glm::vec2& currentPos, const glm::vec2& blockedMove, const glm::vec2& offset, float radius, uint32_t playerID);
    static glm::vec2 GetAverageSurfaceNormal(const glm::vec2& playerCenter, float searchRadius, uint32_t playerID);
    static bool IsPositionSafe(const glm::vec2& pos, const glm::vec2& offset, float radius, uint32_t playerID);
    static glm::vec2 PerformCollisionResponse(const glm::vec2& startPos, const glm::vec2& desiredVelocity, const glm::vec2& offset, float radius, uint32_t playerID);
    static PlayerCollisionSystem::SweepResult SweepCircleAgainstPixels(const glm::vec2& startPos, const glm::vec2& velocity, const glm::vec2& offset, float radius, uint32_t playerID);
    static float SweepCircleVsPoint(const glm::vec2& circleStart, const glm::vec2& circleVelocity, float circleRadius, const glm::vec2& point);
    static bool IsPositionValid(const glm::vec2& pos, const glm::vec2& offset, float radius, uint32_t playerID);
    static glm::vec2 TryMoveWithSubsteps(const glm::vec2& startPos, const glm::vec2& moveVector, const glm::vec2& offset, float radius, uint32_t playerID, int binaryIterations, float minStepSize, int maxSubsteps);
    static float BinarySearchSafeDistance(const glm::vec2& startPos, const glm::vec2& moveDir, float maxDistance, const glm::vec2& offset, float radius, uint32_t playerID, int iterations);
    static bool HasCollisionStrict(const glm::vec2& pos, const glm::vec2& offset, float radius, uint32_t playerID);
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

