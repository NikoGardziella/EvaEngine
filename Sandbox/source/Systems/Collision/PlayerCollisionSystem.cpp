#include "PlayerCollisionSystem.h"
#include "Engine/Scene/Scene.h"
#include <Engine/Scene/Components/Player/CharacterControllerComponent.h>
#include <Engine/Debug/Instrumentor.h>
#include <Engine/Events/Public/CollisionEvents.h>


void PlayerCollisionSystem::UpdatePlayerCollision(entt::registry& registry, float deltaTime, Engine::Scene* scene)
{
    EE_PROFILE_FUNCTION();

    auto staticView = registry.view<Engine::TransformComponent, Engine::BoxCollider2DComponent>();
    auto playerView = registry.view<Engine::TransformComponent, CharacterControllerComponent, Engine::CircleCollider2DComponent, Engine::IDComponent>();

    for (auto playerEntity : playerView)
    {
        auto& playerTransform = playerView.get<Engine::TransformComponent>(playerEntity);
        auto& controller = playerView.get<CharacterControllerComponent>(playerEntity);
        auto& playerCollider = playerView.get<Engine::CircleCollider2DComponent>(playerEntity);
        auto& playerIDComp = playerView.get<Engine::IDComponent>(playerEntity);

        glm::vec2 startPos = glm::vec2(playerTransform.Translation);
        glm::vec2 desiredMove = controller.velocity * deltaTime * controller.speed;
        glm::vec2 offset = playerCollider.Offset;
        float radius = playerCollider.Radius;

        const int steps = 5;  // Number of incremental movement steps
        glm::vec2 stepMove = desiredMove / static_cast<float>(steps);
        glm::vec2 currentPos = startPos;

        bool blocked = false;

        for (int i = 0; i < steps; i++)
        {
            glm::vec2 testPos = currentPos + stepMove;
            glm::vec2 playerCenter = testPos + offset;

            bool collides = false;

            // Check collisions at the test position
            for (const auto& collision : Engine::CollisionResultsCPU::Latest)
            {
                if (playerIDComp.ID != collision.GetProjectileID())
                    continue;

                glm::vec2 collisionPos = collision.HitPosition;

                float dist = glm::distance(playerCenter, collisionPos);

                if (dist < radius)
                {
                    collides = true;
                    break;
                }
            }

            if (collides)
            {
                blocked = true;
                break;  // Stop moving forward, collision ahead
            }
            else
            {
                currentPos = testPos;  // Safe to move forward
            }
        }

        // Set position to last safe position after stepping
        playerTransform.Translation = glm::vec3(currentPos, playerTransform.Translation.z);

        // After stepping, do pushback correction if needed
        glm::vec2 totalPush(0.0f);
        int pushCount = 0;
        glm::vec2 playerCenter = glm::vec2(playerTransform.Translation) + offset;
        for (const auto& collision : Engine::CollisionResultsCPU::Latest)
        {
            if (playerIDComp.ID != collision.GetProjectileID())
                continue;

            glm::vec2 collisionPos = collision.HitPosition;
            float dist = glm::distance(playerCenter, collisionPos);

            if (dist < radius && dist > 0.0001f)
            {
                glm::vec2 pushDir = glm::normalize(playerCenter - collisionPos);
                float penetration = radius - dist;

                totalPush += pushDir * penetration;
                pushCount++;

                // If player is moving into the collision, cancel velocity in that direction
                glm::vec2 velocityDir = glm::length(controller.velocity) > 0.001f
                    ? glm::normalize(controller.velocity) : glm::vec2(0.0f);
                float dot = glm::dot(pushDir, velocityDir);
                if (dot > 0.0f) // Moving toward the collision
                {
                    float velIntoWall = glm::dot(controller.velocity, pushDir);
                    if (velIntoWall < 0.0f)
                        controller.velocity -= pushDir * velIntoWall;
                }
            }
        }

        if (pushCount > 0)
        {
            float pushbackStrength = 1.0f;
            glm::vec2 avgPush = (totalPush / static_cast<float>(pushCount)) * pushbackStrength;

            playerTransform.Translation += glm::vec3(avgPush, 0.0f);
        }

    }

}







PlayerCollisionSystem::RaycastHit PlayerCollisionSystem::SweptCircleAABB(glm::vec2 circleCenter, float radius, glm::vec2 velocity, glm::vec2 aabbMin, glm::vec2 aabbMax)
{
    RaycastHit hit;

    // Expand AABB by radius
    aabbMin -= glm::vec2(radius);
    aabbMax += glm::vec2(radius);

    glm::vec2 invDir = 1.0f / velocity;
    glm::vec2 tMin = (aabbMin - circleCenter) * invDir;
    glm::vec2 tMax = (aabbMax - circleCenter) * invDir;

    if (invDir.x < 0.0f) std::swap(tMin.x, tMax.x);
    if (invDir.y < 0.0f) std::swap(tMin.y, tMax.y);

    float entry = glm::max(tMin.x, tMin.y);
    float exit = glm::min(tMax.x, tMax.y);

    if (entry > exit || (tMin.x < 0.0f && tMin.y < 0.0f))
        return hit;

    hit.t = entry;
    hit.hit = true;

    // Determine collision normal
    if (tMin.x > tMin.y)
        hit.normal = glm::vec2(invDir.x < 0.0f ? 1.0f : -1.0f, 0.0f);
    else
        hit.normal = glm::vec2(0.0f, invDir.y < 0.0f ? 1.0f : -1.0f);

    return hit;
}
