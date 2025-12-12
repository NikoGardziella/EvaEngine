#include "ProjectileSystem.h"
#include <Engine/Debug/Instrumentor.h>
#include <Engine/Scene/Components/Combat/HealthComponent.h>
#include <Engine/Events/Public/CollisionEvents.h>
#include <Engine/Scene/Components/Projectiles/ProjectileComponent.h>
#include <Engine/Scene/Scene.h>
#include <Engine/Core/Log.h>
#include <Engine/Scene/Components/Render/3D/MeshRefComponent.h>
#include <Engine/Scene/Components/Render/3D/SkeletonComponent.h>
#include "Engine/AssetManager/AssetManager.h"
#include <Engine/Scene/Components/Render/3D/RenderBoundsComponent.h>
#include <Engine/Scene/Components/Physics/PhysicsComponent.h>
#include "glm/glm.hpp"

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
        [&](Engine::Entity projectileEntity,
            Engine::TransformComponent& projectileTransformComp,
            ProjectileComponent& projectileComp,
            Engine::IDComponent& projectileIdComp)
        {
            // 1) Check if this projectile had a GPU collision
            bool gpuHit = false;

            for (const auto& col : gpuCollisions)
            {
                if (projectileIdComp.ID == col.GetEntityID())
                {
                    gpuHit = true;
                    break;
                }
            }

            // 2) Integrate projectile movement
            projectileTransformComp.Translation.x += projectileComp.Direction.x * deltaTime * projectileComp.ProjectileSped;
            projectileTransformComp.Translation.y += projectileComp.Direction.y * deltaTime * projectileComp.ProjectileSped;

            const glm::vec2 projectilePos = {
                projectileTransformComp.Translation.x,
                projectileTransformComp.Translation.y
            };



           

            // 3) Apply blast radius ONLY if we had a GPU hit
            const float blastRadius = projectileComp.DestructionRadius; // e.g. 0.1f

            bool hitSomething = false;

            scene->ForEach<Engine::TransformComponent>(
                [&](Engine::Entity targetEntity, Engine::TransformComponent& targetTransformComp)
                {
                    if (hitSomething) return;                          // already processed a main hit
                    if (targetEntity == projectileEntity) return;      // skip self
                    if (targetEntity == projectileComp.Owner) return;  // skip owner

                    bool hit = false;

                    if (!hit && targetEntity.HasComponent<Engine::EnemyDestructibleComponent>())
                    {
                        auto& destr = targetEntity.GetComponent<Engine::EnemyDestructibleComponent>();

                        float bestDist2 = std::numeric_limits<float>::max();
                        Engine::EnemyPieceType bestPiece = Engine::EnemyPieceType::Torso;
                        bool anyHit = false;

                        glm::mat4 enemyWorld = targetTransformComp.GetTransform();

                        for (const auto& piece : destr.pieces)
                        {
                            if (!piece.hitEnabled)
                                continue;

                            if (piece.hitShape != Engine::HitVolumeShape::Sphere)
                                continue;

                            if (piece.detached)
                                continue;

                            // World-space center of piece
                            glm::vec4 centerW4 = enemyWorld * glm::vec4(piece.hitLocalCenter, 1.0f);
                            glm::vec2 centerW2(centerW4.x, centerW4.y);

                            glm::vec2 diff = projectilePos - centerW2;
                            float dist2 = glm::dot(diff, diff);

                            // Explosion sphere vs piece sphere

                            float totalRadius;
                            if (gpuHit)
                            {
                                totalRadius = piece.hitRadius + blastRadius;
                            }
                            else
                            {
                                totalRadius = piece.hitRadius;
                            }

                             
                            float r2 = totalRadius * totalRadius;

                            if (dist2 <= r2 && dist2 < bestDist2)
                            {
                                bestDist2 = dist2;
                                bestPiece = piece.type;
                                anyHit = true;
                            }
                        }

                        if (anyHit)
                        {
                            hit = true;
                            hitSomething = true;

                            glm::vec3 impulseDir = glm::vec3(projectileComp.Direction, 0.0f);
                            float impulseStrength = 10.0f;

                            DetachPiece(scene, targetEntity, bestPiece, impulseDir, impulseStrength);
                            toDestroy.push_back(projectileEntity);
                            return;
                        }
                    }

                    const glm::vec2 targetPos = { targetTransformComp.Translation.x, targetTransformComp.Translation.y };



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

                

                    // 4) Apply damage if available
                    if (hit && targetEntity.HasComponent<Engine::HealthComponent>())
                    {
                        auto& healthComp = targetEntity.GetComponent<Engine::HealthComponent>();
                        healthComp.Current -= projectileComp.Damage;


                    }

                   
                });

            // Lifetime expiry
            // this is now in time when it should be distance.
            // CHANGE 
            projectileComp.ProjectileMaxRange -= deltaTime;
            if (projectileComp.ProjectileMaxRange <= 0.0f || gpuHit)
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

inline Engine::EnemyPiece* ProjectileSystem::FindPiece(Engine::EnemyDestructibleComponent& destr, Engine::EnemyPieceType type)
{
    for (auto& p : destr.pieces)
        if (p.type == type)
            return &p;
    return nullptr;
}



void ProjectileSystem::DetachPiece(Engine::Scene* scene, Engine::Entity enemy, Engine::EnemyPieceType type,  const glm::vec3& impulseDir,
    float impulseStrength)
{
    if (!enemy.HasComponent<Engine::EnemyDestructibleComponent>())
        return;

    Engine::EnemyDestructibleComponent& destr = enemy.GetComponent<Engine::EnemyDestructibleComponent>();

    Engine::EnemyPiece* piece = FindPiece(destr, type);
    if (!piece)
        return;

    if (!piece->visible || !piece->canDetach || piece->detached)
        return;

    // 1) Hide piece on the main enemy
    piece->visible = 0;
    piece->detached = 1;
    piece->hitEnabled = 0;

    // 2) Get required components
    if (!enemy.HasComponent<Engine::TransformComponent>() ||
        !enemy.HasComponent<Engine::MeshRefComponent>() ||
        !enemy.HasComponent<Engine::SkeletonComponent>())
    {
        return;
    }

    Engine::TransformComponent& xform = enemy.GetComponent<Engine::TransformComponent>();
    Engine::MeshRefComponent& meshRef = enemy.GetComponent<Engine::MeshRefComponent>();
    Engine::SkeletonComponent& skel = enemy.GetComponent<Engine::SkeletonComponent>();

    const Engine::MeshAsset& mesh = Engine::AssetManager::GetMeshRegistry().GetMesh(meshRef.meshId);

    if (piece->submeshIndex >= mesh.submeshes.size())
    {
        EE_CORE_WARN("[EnemyDestructible] submeshIndex {} out of range (mesh has {})",
            piece->submeshIndex, mesh.submeshes.size());
        return;
    }


    glm::mat4 enemyWorld = xform.GetTransform();

    glm::vec3 spawnPos = glm::vec3(enemyWorld[3]);

    glm::vec3 extraOffset(0.0f);

    switch (type)
    {
    case Engine::EnemyPieceType::Head:
        extraOffset = glm::vec3(0.0f, 0.8f, 0.0f); // up
        break;
    case Engine::EnemyPieceType::ArmL:
        extraOffset = glm::vec3(-0.4f, 0.4f, 0.0f);
        break;
    case Engine::EnemyPieceType::ArmR:
        extraOffset = glm::vec3(+0.4f, 0.4f, 0.0f);
        break;
    case Engine::EnemyPieceType::LegL:
        extraOffset = glm::vec3(-0.2f, -0.6f, 0.0f);
        break;
    case Engine::EnemyPieceType::LegR:
        extraOffset = glm::vec3(+0.2f, -0.6f, 0.0f);
        break;
    default:
        break;
    }

    glm::vec3 worldOffset = glm::vec3(enemyWorld * glm::vec4(extraOffset, 0.0f));
    spawnPos += worldOffset;

    Engine::Entity gib = scene->CreateEntity("DetachedPiece");

    Engine::TransformComponent& gx = gib.AddComponent<Engine::TransformComponent>();
    gx.Translation = spawnPos;
    gx.Rotation = xform.Rotation;
    gx.Scale = xform.Scale;

    Engine::MeshRefComponent& gMeshRef = gib.AddComponent<Engine::MeshRefComponent>();
    gMeshRef.meshId = meshRef.meshId;
    gMeshRef.submeshFirst = piece->submeshIndex;
    gMeshRef.submeshCount = 1;

    Engine::RenderBoundsComponent& bounds = gib.AddComponent<Engine::RenderBoundsComponent>();
    bounds.minL = mesh.submeshes[piece->submeshIndex].aabbMin;
    bounds.maxL = mesh.submeshes[piece->submeshIndex].aabbMax;


    PhysicsComponent& phys = gib.AddComponent<PhysicsComponent>();
    phys.velocity = glm::normalize(impulseDir) * impulseStrength;
    phys.gravity = glm::vec3(0.0f, -9.8f, 0.0f);
    phys.active = true;

    float movementTimer = 0.5f;
    phys.duration = movementTimer;
    phys.timeLeft = movementTimer;
    phys.randomizedSpin = true;

    uint32_t skeletonId = 0;
    Engine::SkeletonComponent& newSkeleton = gib.AddComponent<Engine::SkeletonComponent>();
    newSkeleton.skeletonId = skeletonId;
    newSkeleton.boneCount = Engine::AssetManager::GetSkeletonRegistry().Get(skeletonId).parent.size();
    newSkeleton.boneBase = 0xFFFFFFFFu;
}