#include "ProjectileSystem.h"
#include "Engine.h"
#include <Engine/Debug/Instrumentor.h>
#include <Engine/Scene/Components/Combat/HealthComponent.h>
#include <Engine/Events/Public/CollisionEvents.h>
#include <Engine/Scene/Components/Projectiles/ProjectileComponent.h>

#include <Engine/Scene/Components/Render/3D/MeshRefComponent.h>
#include <Engine/Scene/Components/Render/3D/SkeletonComponent.h>
#include "Engine/AssetManager/AssetManager.h"
#include <Engine/Scene/Components/Render/3D/RenderBoundsComponent.h>
#include <Engine/Scene/Components/Physics/PhysicsComponent.h>

#include <Engine/Scene/Components/Animation/NpcAnimationControllerComponent.h>

#include "Engine/Renderer/Renderer2D/VulkanRenderer2D.h"
void ProjectileSystem::UpdateProjectileSystem(float deltaTime, Engine::Scene* scene)
{
    EE_PROFILE_FUNCTION();

    std::unordered_set<Engine::UUID> gpuHitIDs;
    for (const auto& col : Engine::CollisionResultsCPU::LatestProjectiles)
        gpuHitIDs.insert(col.GetEntityID());

    std::vector<Engine::Entity> projectilesToDestroy;


    scene->ForEach<Engine::TransformComponent, ProjectileComponent, Engine::IDComponent>(
        [&](Engine::Entity projectileEntity, Engine::TransformComponent& transformComp,
            ProjectileComponent& projectileComp, Engine::IDComponent& idComp)
        {
            IntegrateMovement(projectileComp, transformComp, deltaTime);

            const glm::vec2 projectilePos = { transformComp.Translation.x, transformComp.Translation.y };
            const bool gpuHit = gpuHitIDs.count(idComp.ID);
            const bool outOfRange = projectileComp.DistanceTravelled >= projectileComp.ProjectileMaxRange;
            const bool hitTarget = projectileComp.DistanceTravelled >= projectileComp.DistanceToTargetatFireTime;

            if (outOfRange || hitTarget)
                Engine::VulkanRenderer2D::SubmitCPUExplosion(projectilePos, projectileComp.DestructionRadius, projectileComp.Damage);

            bool destroyProjectile = outOfRange || gpuHit || hitTarget;

            scene->ForEach<Engine::TransformComponent>(
                [&](Engine::Entity targetEntity, Engine::TransformComponent& targetTransform)
                {
                    if (targetEntity == projectileEntity)       return;
                    if (targetEntity == projectileComp.Owner)  return;

                    if (IsInBlastRadius(projectilePos, projectileComp, targetEntity, targetTransform))
                    {
                        ApplyBlastToEntity(scene, projectilePos, projectileComp, targetEntity, targetTransform);
                        destroyProjectile = true;
                    }
                });

            if (destroyProjectile)
                projectilesToDestroy.push_back(projectileEntity);
        });

    for (auto& e : projectilesToDestroy)
        scene->DestroyEntity(e);
}

void ProjectileSystem::IntegrateMovement(ProjectileComponent& projectileComp, Engine::TransformComponent& transformComp, float deltaTime)
{
    const float stepDist = projectileComp.ProjectileSped * deltaTime;
    transformComp.Translation.x += projectileComp.Direction.x * stepDist;
    transformComp.Translation.y += projectileComp.Direction.y * stepDist;
    projectileComp.DistanceTravelled += stepDist;
}

bool ProjectileSystem::IsInBlastRadius(const glm::vec2& projectilePos, const ProjectileComponent& projectileComp,
    Engine::Entity targetEntity, Engine::TransformComponent& targetTransform)
{
    const glm::vec2 targetPos = { targetTransform.Translation.x, targetTransform.Translation.y };
    const float blastRadius = projectileComp.DestructionRadius;

    if (targetEntity.HasComponent<Engine::EnemyDestructibleComponent>())
    {
        const glm::mat4 enemyWorld = targetTransform.GetTransform();
        const auto& destr = targetEntity.GetComponent<Engine::EnemyDestructibleComponent>();

        for (const auto& piece : destr.pieces)
        {
            if (!piece.hitEnabled || piece.detached || piece.hitShape != Engine::HitVolumeShape::Sphere)
            {

                continue;
            }

            glm::vec2 centerW = glm::vec2(enemyWorld * glm::vec4(piece.hitLocalCenter, 1.0f));
            float totalRadius = piece.hitRadius + blastRadius;

            if (glm::distance2(projectilePos, centerW) <= totalRadius * totalRadius)
            {

                return true;
            }
        }
    }

    if (targetEntity.HasComponent<Engine::BoxCollider2DComponent>())
    {
        const auto& box = targetEntity.GetComponent<Engine::BoxCollider2DComponent>();
        const glm::vec2 half = 0.5f * box.Size;
        const glm::vec2 minB = targetPos - half;
        const glm::vec2 maxB = targetPos + half;

        if (projectilePos.x >= minB.x && projectilePos.x <= maxB.x &&
            projectilePos.y >= minB.y && projectilePos.y <= maxB.y)
            {
                return true;
            }
    }

    if (targetEntity.HasComponent<Engine::CircleCollider2DComponent>())
    {
        const auto& circle = targetEntity.GetComponent<Engine::CircleCollider2DComponent>();
        if (glm::distance2(projectilePos, targetPos) <= circle.Radius * circle.Radius)
        {

            return true;
        }
    }

    return false;
}

void ProjectileSystem::ApplyBlastToEntity(Engine::Scene* scene, const glm::vec2& projectilePos, const ProjectileComponent& projectileComp, 
    Engine::Entity targetEntity, Engine::TransformComponent& targetTransform)
{
    const float blastRadius = projectileComp.DestructionRadius;
    const glm::vec3 impulseDir = glm::vec3(projectileComp.Direction, 0.0f);
    const float impulseStrength = blastRadius * 2.0f;

    if (targetEntity.HasComponent<Engine::EnemyDestructibleComponent>())
    {
        Engine::EnemyPieceType bestPiece = FindClosestPiece(projectilePos, projectileComp, targetEntity, targetTransform);
        DetachPiece(scene, targetEntity, bestPiece, impulseDir, impulseStrength);
    }

    ApplyImpulse(targetEntity, impulseDir, impulseStrength);

    if (targetEntity.HasComponent<HealthComponent>())
    {
        Engine::VulkanRenderer2D::SubmitCPUExplosion(projectilePos, blastRadius, projectileComp.Damage);

        HealthComponent& health = targetEntity.GetComponent<HealthComponent>();
        health.Current -= projectileComp.Damage;

        NpcAnimationControllerComponent& anim = targetEntity.GetComponent<NpcAnimationControllerComponent>();
        anim.request = (health.Current > 0) ? NpcAnimRequest::Hit : NpcAnimRequest::Death;
    }
}

Engine::EnemyPieceType ProjectileSystem::FindClosestPiece(const glm::vec2& projectilePos, const ProjectileComponent& projectileComp,
    Engine::Entity targetEntity, Engine::TransformComponent& targetTransform)
{
    const auto& destr = targetEntity.GetComponent<Engine::EnemyDestructibleComponent>();
    const glm::mat4 world = targetTransform.GetTransform();
    const float blastRadius = projectileComp.DestructionRadius;

    float bestDist2 = std::numeric_limits<float>::max();
    Engine::EnemyPieceType bestPiece = Engine::EnemyPieceType::Torso;

    for (const auto& piece : destr.pieces)
    {
        if (!piece.hitEnabled || piece.detached || piece.hitShape != Engine::HitVolumeShape::Sphere)
        {

            continue;
        }

        glm::vec2 centerW = glm::vec2(world * glm::vec4(piece.hitLocalCenter, 1.0f));
        float dist2 = glm::distance2(projectilePos, centerW);
        float totalRadius = piece.hitRadius + blastRadius;

        if (dist2 <= totalRadius * totalRadius && dist2 < bestDist2)
        {
            bestDist2 = dist2;
            bestPiece = piece.type;
        }
    }

    return bestPiece;
}

void ProjectileSystem::ApplyImpulse(Engine::Entity targetEntity, const glm::vec3& impulseDir, float impulseStrength)
{
    if (targetEntity.HasComponent<PhysicsComponent>())
        return;

    PhysicsComponent& phys = targetEntity.AddComponent<PhysicsComponent>();
    phys.velocity = glm::normalize(impulseDir) * impulseStrength;
    phys.gravity = glm::vec3(0.0f, -9.8f, 0.0f);
    phys.active = true;
    phys.removeOnFinish = true;
    phys.duration = 0.5f;
    phys.timeLeft = 0.5f;
}

inline Engine::EnemyPiece* ProjectileSystem::FindPiece(Engine::EnemyDestructibleComponent& destr, Engine::EnemyPieceType type)
{
    for (auto& p : destr.pieces)
    {
        if (p.type == type)
        {
            return &p;
        }
    }
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


    const auto& sm = mesh.submeshes[piece->submeshIndex];
    glm::vec3 centerL = 0.5f * (sm.aabbMin + sm.aabbMax);
    glm::vec3 spawnPos = glm::vec3(enemyWorld * glm::vec4(centerL, 1.0f));




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


    // this should be removd
    Engine::SkeletonComponent& newSkeleton = gib.AddComponent<Engine::SkeletonComponent>();
    newSkeleton.skeletonId = skeletonId;
    newSkeleton.boneCount = Engine::AssetManager::GetSkeletonRegistry().Get(skeletonId).parent.size();
    newSkeleton.boneBase = 0xFFFFFFFFu;
}