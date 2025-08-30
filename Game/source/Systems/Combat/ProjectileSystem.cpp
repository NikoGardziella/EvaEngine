#include "ProjectileSystem.h"
#include <Engine/Debug/Instrumentor.h>
#include <Engine/Scene/Components/Combat/HealthComponent.h>
#include <Engine/Events/Public/CollisionEvents.h>
#include <Engine/Scene/Components/Projectiles/ProjectileComponent.h>
#include <Engine/Scene/Scene.h>


void ProjectileSystem::UpdateProjectileSystem(float deltaTime, Engine::Scene* scene)
{
    EE_PROFILE_FUNCTION();

    // Cache GPU collision results for this tick
    const auto gpuCollisions = Engine::CollisionResultsCPU::LatestProjectiles;

    // Collect projectiles to destroy after iteration (safer with ECS wrappers)
    std::vector<Engine::Entity> toDestroy;
    toDestroy.reserve(64);

    // Update & collide all projectiles
    scene->ForEach<Engine::TransformComponent, ProjectileComponent, Engine::IDComponent>(
        [&](Engine::Entity projectileEntity, Engine::TransformComponent& projectileTransformComp,
            ProjectileComponent& projectileComp,  Engine::IDComponent& projectileIdComp)
        {
            for (const auto& col : gpuCollisions)
            {
                if (projectileIdComp.ID == col.GetEntityID())
                {
                    EE_INFO("Projectile collided (GPU)");
                    toDestroy.push_back(projectileEntity);
                    return; // stop processing this projectile
                }
            }

            constexpr float kProjectileSpeed = 10.0f;
            projectileTransformComp.Translation.x += projectileComp.Direction.x * deltaTime * kProjectileSpeed;
            projectileTransformComp.Translation.y += projectileComp.Direction.y * deltaTime * kProjectileSpeed;

            const glm::vec2 projectilePos = {
                projectileTransformComp.Translation.x,
                projectileTransformComp.Translation.y
            };

            //  Broad/narrow phase vs scene entities (very simple AABB/circle checks)
            bool hitSomething = false;

            scene->ForEach<Engine::TransformComponent>(
                [&](Engine::Entity targetEntity, Engine::TransformComponent& targetTransformComp)
                {
                    if (hitSomething) return;                          // already hit this frame
                    if (targetEntity == projectileEntity) return;      // skip self
                    if (targetEntity == projectileComp.Owner) return;  // skip owner

                    const glm::vec2 targetPos = {
                        targetTransformComp.Translation.x,
                        targetTransformComp.Translation.y
                    };

                    bool hit = false;

                    // Box collider
                    if (!hit && targetEntity.HasComponent<Engine::BoxCollider2DComponent>())
                    {
                        const auto& box = targetEntity.GetComponent<Engine::BoxCollider2DComponent>();
                        const glm::vec2 half = 0.5f * box.Size;
                        const glm::vec2 minB = targetPos - half;
                        const glm::vec2 maxB = targetPos + half;

                        if (projectilePos.x >= minB.x && projectilePos.x <= maxB.x &&
                            projectilePos.y >= minB.y && projectilePos.y <= maxB.y)
                        {
                            hit = true;
                        }
                    }

                    // Circle collider
                    if (!hit && targetEntity.HasComponent<Engine::CircleCollider2DComponent>())
                    {
                        const auto& circle = targetEntity.GetComponent<Engine::CircleCollider2DComponent>();
                        const float r2 = circle.Radius * circle.Radius;
                        if (glm::distance2(projectilePos, targetPos) <= r2)
                        {
                            hit = true;
                        }
                    }

                    if (!hit) return;

                    // 4) Apply damage if available
                    if (targetEntity.HasComponent<Engine::HealthComponent>())
                    {
                        auto& healthComp = targetEntity.GetComponent<Engine::HealthComponent>();
                        healthComp.Current -= projectileComp.Damage;
                    }

                    // Mark projectile for destroy
                    hitSomething = true;
                    toDestroy.push_back(projectileEntity);
                });

            if (hitSomething) return;

            // Lifetime expiry
            // this is now in time when it should be distance.
            // CHANGE 
            projectileComp.ProjectileMaxRange -= deltaTime;
            if (projectileComp.ProjectileMaxRange <= 0.0f)
            {
                toDestroy.push_back(projectileEntity);
                return;
            }
        });

    // Destroy all marked projectiles after iteration
    for (auto& e : toDestroy)
    {
        scene->DestroyEntity(e);
    }
}
