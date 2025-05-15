#include "PlayerCollisionSystem.h"
#include "Engine/Scene/Scene.h"
#include <Engine/Scene/Components/Player/CharacterControllerComponent.h>
#include <Engine/Debug/Instrumentor.h>


void PlayerCollisionSystem::UpdatePlayerCollision(entt::registry& registry, float deltaTime, Engine::Scene* scene)
{
    EE_PROFILE_FUNCTION();

    auto staticView = registry.view<Engine::TransformComponent, Engine::BoxCollider2DComponent>();
    auto playerView = registry.view<Engine::TransformComponent, CharacterControllerComponent, Engine::CircleCollider2DComponent>();

    for (auto playerEntity : playerView)
    {
        auto& playerTransform = playerView.get<Engine::TransformComponent>(playerEntity);
        auto& controller = playerView.get<CharacterControllerComponent>(playerEntity);
        auto& playerCollider = playerView.get<Engine::CircleCollider2DComponent>(playerEntity);

        glm::vec2 startPos = glm::vec2(playerTransform.Translation);
        glm::vec2 velocity = controller.velocity;
        glm::vec2 attemptedMove = velocity * deltaTime * controller.speed;

        glm::vec2 offset = playerCollider.Offset;
        float radius = playerCollider.Radius;

        auto IsColliding = [&](glm::vec2 testPos) -> bool
            {
                glm::vec2 minA = testPos + offset - glm::vec2(radius);
                glm::vec2 maxA = testPos + offset + glm::vec2(radius);

                for (auto otherEntity : staticView)
                {
                    auto& otherTransform = staticView.get<Engine::TransformComponent>(otherEntity);
                    auto& otherBox = staticView.get<Engine::BoxCollider2DComponent>(otherEntity);

                    glm::vec2 boxScale = glm::vec2(otherTransform.Scale);
                    glm::vec2 boxCenter = glm::vec2(otherTransform.Translation) + otherBox.Offset * boxScale;
                    glm::vec2 boxHalfSize = (otherBox.Size * boxScale);

                    glm::vec2 minB = boxCenter - boxHalfSize;
                    glm::vec2 maxB = boxCenter + boxHalfSize;

                    bool overlapX = maxA.x > minB.x && minA.x < maxB.x;
                    bool overlapY = maxA.y > minB.y && minA.y < maxB.y;

                    if (overlapX && overlapY)
                        return true;
                }

                return false;
            };

        glm::vec2 newPos = startPos + attemptedMove;

        if (!IsColliding(newPos))
        {
            // Full move is fine
            playerTransform.Translation = glm::vec3(newPos, playerTransform.Translation.z);
        }
        else
        {
            // Try move on X axis only
            glm::vec2 xMove = startPos + glm::vec2(attemptedMove.x, 0.0f);
            bool xFree = !IsColliding(xMove);

            // Try move on Y axis only
            glm::vec2 yMove = startPos + glm::vec2(0.0f, attemptedMove.y);
            bool yFree = !IsColliding(yMove);

            if (xFree)
            {
                playerTransform.Translation = glm::vec3(xMove, playerTransform.Translation.z);
            }

            if (yFree)
            {
                playerTransform.Translation.y = yMove.y;
            }

            // Stop movement in blocked directions
            if (!xFree) controller.velocity.x = 0.0f;
            if (!yFree) controller.velocity.y = 0.0f;
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
