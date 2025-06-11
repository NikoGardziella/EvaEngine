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

        const int steps = 5;
        glm::vec2 stepMove = desiredMove / static_cast<float>(steps);
        glm::vec2 currentPos = startPos;
        for (int i = 0; i < steps; i++)
        {
            glm::vec2 testPos = currentPos + stepMove;
            glm::vec2 playerCenter = testPos + offset;

            bool collides = false;
            glm::vec2 firstCollisionPos;

            // Check initial move collision
            for (const auto& collision : Engine::CollisionResultsCPU::LatestProjectiles)
            {
                if (playerIDComp.ID != collision.GetProjectileID())
                    continue;

                glm::vec2 collisionPos = collision.HitPosition;
                float dist = glm::distance(playerCenter, collisionPos);

                if (dist < radius)
                {
                    collides = true;
                    firstCollisionPos = collisionPos;
                    break;
                }
            }

            if (collides)
            {
                glm::vec2 velocityDir = glm::length(stepMove) > 0.001f ? glm::normalize(stepMove) : glm::vec2(0.0f);
                glm::vec2 pushDir = glm::normalize(playerCenter - firstCollisionPos);

                // Compute slide vector: original minus projection onto obstacle normal
                glm::vec2 slideDir = velocityDir - glm::dot(velocityDir, pushDir) * pushDir;

                // Try sliding in both directions
                glm::vec2 slideDirs[2] = {
                    slideDir,
                    -slideDir
                };

                bool slideSucceeded = false;

                for (int dirIndex = 0; dirIndex < 2; ++dirIndex)
                {
                    glm::vec2 trySlideDir = slideDirs[dirIndex];
                    if (glm::length(trySlideDir) < 0.001f)
                        continue;

                    trySlideDir = glm::normalize(trySlideDir);
                    glm::vec2 slideMove = trySlideDir * glm::length(stepMove) * 0.9f; // Slightly shorter step to avoid edge snagging

                    glm::vec2 slideTestPos = currentPos + slideMove;
                    glm::vec2 slideCenter = slideTestPos + offset;

                    bool slideCollides = false;
                    for (const auto& collision : Engine::CollisionResultsCPU::LatestProjectiles)
                    {
                        if (playerIDComp.ID != collision.GetProjectileID())
                            continue;

                        glm::vec2 collisionPos = collision.HitPosition;
                        float dist = glm::distance(slideCenter, collisionPos);

                        if (dist < radius)
                        {
                            slideCollides = true;
                            break;
                        }
                    }

                    if (!slideCollides)
                    {
                        currentPos += slideMove;
                        slideSucceeded = true;
                        break;
                    }
                }

                if (!slideSucceeded)
                    break; // Neither slide direction worked
            }
            else
            {
                currentPos = testPos; // No collision, normal step
            }
        }


        // Apply final position
        playerTransform.Translation = glm::vec3(currentPos, playerTransform.Translation.z);

        // Pushback correction (as in your original code)
        glm::vec2 totalPush(0.0f);
        int pushCount = 0;
        glm::vec2 playerCenter = glm::vec2(playerTransform.Translation) + offset;
        for (const auto& collision : Engine::CollisionResultsCPU::LatestProjectiles)
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

                // Cancel velocity toward collision
                glm::vec2 velocityDir = glm::length(controller.velocity) > 0.001f ? glm::normalize(controller.velocity) : glm::vec2(0.0f);
                float dot = glm::dot(pushDir, velocityDir);
                if (dot > 0.0f)
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
