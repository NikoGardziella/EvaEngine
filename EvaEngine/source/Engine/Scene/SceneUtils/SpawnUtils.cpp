#include "pch.h"
#include "SpawnUtils.h"
#include <Engine/AssetManager/AssetManager.h>
#include <Engine/Animation/3D/AnimationRegistry.h>
#include <Engine/Scene/Components/Render/3D/AnimatorComponent.h>
#include <Engine/Animation/3D/SkeletonRegistry.h>
#include "Prefabs/NPCprefab.h"
#include <Engine/Scene/Scene.h>
#include <Engine/Scene/Components/Combat/HealthComponent.h>
#include <Engine/Scene/Components/Render/3D/MeshRefComponent.h>
#include <Engine/Scene/Components/Render/3D/RenderBoundsComponent.h>
#include <Engine/Scene/Components/Render/3D/SkeletonComponent.h>
#include "Engine/Scene/SceneUtils/SceneUtils.h"
#include <random>
#include <Engine/Scene/Components/NPC/NpcBodyStateComponent.h>

namespace Engine {

    uint32_t SpawnUtils::FindClipId(const char* name)
    {
        auto& animReg = Engine::AssetManager::GetAnimationRegistry();
        if (const Engine::AnimationClip* c = animReg.FindAnimationClip(name))
            return c->id;
        return 0xFFFFFFFFu;
    }





    
    uint32_t SpawnUtils::FindClipIdChecked(const char* name,  uint32_t expectedSkeletonId, uint32_t expectedBoneCount)
    {
        static constexpr uint32_t kInvalidId = 0xFFFFFFFFu;

        auto& animReg = Engine::AssetManager::GetAnimationRegistry();

        const Engine::AnimationClip* animClip = animReg.FindAnimationClip(name);
        if (!animClip)
        {
            return kInvalidId;
        }

        const SkeletonAsset& skeletonAsset = AssetManager::GetSkeletonRegistry().Get(animClip->skeletonId);
        const uint32_t animBoneCount = (uint32_t)skeletonAsset.restLocal.size();

        

        if (expectedBoneCount != 0)
        {
            // If  clip stores bone/joint count, check it too:
            if (animBoneCount != expectedBoneCount) //
            {
                EE_CORE_WARN("[Spawn] Clip '{}' bone count mismatch. clip.boneCount={} expected={}",
                    animBoneCount, expectedBoneCount);
                return kInvalidId;
            }
        }

        return animClip->id;
    }


    // now there is skeletonId for each animation. Best way would probably be to attach animation to a mesh
    // and its skeletonId. Here I just check that the animation and mesh bonecount match.
    void SpawnUtils::ResolveZombieClips(NpcAnimationControllerComponent& animControllerComp,
        const Engine::MeshAsset& mesh)
    {
        static constexpr uint32_t kInvalidId = 0xFFFFFFFFu;

        // 1) Mesh must have a skeleton
        const uint32_t skeletonId = mesh.skeletonId;
        if (skeletonId == kInvalidId)
        {
            EE_CORE_ERROR("[Spawn] Zombie mesh has invalid skeletonId");
            animControllerComp.clipsResolved = false;
            return;
        }

        // 2) Validate mesh skeleton bone count (zombie should be 55. From mixamo)
        const Engine::SkeletonAsset& skel = Engine::AssetManager::GetSkeletonRegistry().Get(skeletonId);


        const uint32_t meshBoneCount = (uint32_t)skel.restLocal.size();

        if (meshBoneCount == 0)
        {
            EE_CORE_WARN("[Spawn] Zombie mesh skeleton bone count is {} (meshId={}, skeletonId={})",
                meshBoneCount, mesh.id, skeletonId);
       
        }

        // 3) Resolve clips with checks against this skeleton/boneCount
        animControllerComp.clipAgonize = FindClipIdChecked("zombieAgonizing", skeletonId, meshBoneCount);
        animControllerComp.clipCrawl = FindClipIdChecked("zombieAnimCrawl", skeletonId, meshBoneCount);
        animControllerComp.clipIdle = FindClipIdChecked("zombieAnimIdle", skeletonId, meshBoneCount);
        animControllerComp.clipWalk = FindClipIdChecked("zombieAnimWalk", skeletonId, meshBoneCount);
        animControllerComp.clipHit = FindClipIdChecked("zombieAnimHeadHit", skeletonId, meshBoneCount);
        animControllerComp.clipDeath = FindClipIdChecked("zombieAnimDeath", skeletonId, meshBoneCount);
        animControllerComp.clipAttack = FindClipIdChecked("zombieAnimStrike", skeletonId, meshBoneCount);
        animControllerComp.clipRun = FindClipIdChecked("zombieAnimRunning", skeletonId, meshBoneCount);
        animControllerComp.clipTrip = FindClipIdChecked("zombieAnimTrip", skeletonId, meshBoneCount);
        animControllerComp.clipStandup = FindClipIdChecked("zombieAnimStandup", skeletonId, meshBoneCount);

        //  fail if any clip is missing
        const bool ok =
            animControllerComp.clipAgonize != kInvalidId &&
            animControllerComp.clipCrawl != kInvalidId &&
            animControllerComp.clipIdle != kInvalidId &&
            animControllerComp.clipWalk != kInvalidId &&
            animControllerComp.clipHit != kInvalidId &&
            animControllerComp.clipDeath != kInvalidId &&
            animControllerComp.clipAttack != kInvalidId &&
            animControllerComp.clipRun != kInvalidId;

        animControllerComp.clipsResolved = ok;

        if (!ok)
        {
            EE_CORE_WARN("wrong animation");
        }

    }


    glm::vec2 SpawnUtils::RandomUnit2D(uint32_t& seed)
    {
        // tiny deterministic RNG per spawner (no system state)
        seed = 1664525u * seed + 1013904223u;
        const float t = (seed & 0xFFFFFF) / float(0x1000000); // [0,1)
        const float ang = t * glm::two_pi<float>();
        return glm::vec2(std::cos(ang), std::sin(ang));
    }

    float SpawnUtils::RandomRange(uint32_t& seed, float a, float b)
    {
        seed = 1664525u * seed + 1013904223u;
        const float t = (seed & 0xFFFFFF) / float(0x1000000);
        return a + (b - a) * t;
    }


    Engine::Entity SpawnUtils::SpawnNPCFromPrefab(Scene* scene, const NpcPrefab& prefab, const glm::vec3& worldPos)
    {
        auto& meshReg = AssetManager::GetMeshRegistry();
        const MeshAsset* meshAsset = meshReg.GetMeshByKey(prefab.meshKey);
        if (!meshAsset)
        {
            EE_CORE_ERROR("[SpawnNPC] Mesh key not found: {}", prefab.meshKey);
            return {};
        }

        const uint32_t submeshCount = (uint32_t)meshAsset->submeshes.size();
        if (submeshCount == 0)
        {
            EE_CORE_ERROR("[SpawnNPC] Mesh has no submeshes: {}", prefab.meshKey);
            return {};
        }

        // SkeletonId: either explicit or from mesh
        uint32_t skeletonId = (prefab.overrideSkeletonId != 0xFFFFFFFFu)
            ? prefab.overrideSkeletonId
            : meshAsset->skeletonId;

        if (skeletonId == 0xFFFFFFFFu)
        {
            EE_CORE_ERROR("[SpawnNPC] Invalid skeletonId for meshKey={}", prefab.meshKey);
            return {};
        }

        const SkeletonAsset& skeletonAsset = AssetManager::GetSkeletonRegistry().Get(skeletonId);

        Engine::Entity enemyNPCEntity = scene->CreateEntity(prefab.name);
        enemyNPCEntity.AddComponent<HealthComponent>();

        // Transform
        TransformComponent& npcTransformComp = enemyNPCEntity.AddComponent<TransformComponent>();
        npcTransformComp.Translation = worldPos;
        npcTransformComp.Rotation.x += glm::radians(prefab.pitchOffsetDeg);
        npcTransformComp.Rotation.z += glm::radians(prefab.yawOffsetDeg);
        npcTransformComp.Scale *= glm::vec3(meshAsset->importScale);

        // Mesh
        MeshRefComponent& meshComp = enemyNPCEntity.AddComponent<MeshRefComponent>();
        meshComp.meshId = meshAsset->id;
        meshComp.submeshFirst = 0;
        meshComp.submeshCount = submeshCount;

        // Render bounds
        RenderBoundsComponent& renderBoundsComp = enemyNPCEntity.AddComponent<RenderBoundsComponent>();
        renderBoundsComp.maxL = meshAsset->maxL;
        renderBoundsComp.minL = meshAsset->minL;

        // Skeleton
        SkeletonComponent& skeletonComp = enemyNPCEntity.AddComponent<SkeletonComponent>();
        skeletonComp.skeletonId = skeletonId;
        skeletonComp.boneCount = (uint32_t)skeletonAsset.parent.size();
        skeletonComp.boneBase = 0xFFFFFFFFu;

        // Animator
        Animator3DComponent& animComp = enemyNPCEntity.AddComponent<Animator3DComponent>();
        animComp.clipA = INVALID_CLIP;
        animComp.clipB = INVALID_CLIP;
        animComp.timeA = 0.0f;
        animComp.timeB = 0.0f;
        animComp.blend = 0.0f;
        animComp.playbackSpeed = 1.0f;

        // Destructible pieces
        EnemyDestructibleComponent& destrComp = enemyNPCEntity.AddComponent<EnemyDestructibleComponent>();
        destrComp.pieces.clear();
        destrComp.pieces.reserve(submeshCount);

        for (uint32_t smi = 0; smi < submeshCount; ++smi)
        {
            const SubmeshRange& sm = meshAsset->submeshes[smi];

            EnemyPiece p{};
            p.submeshIndex = smi;
            p.type = SceneUtils::ClassifyPieceTypeFromSubmeshName(sm.name);
            p.boneId = SceneUtils::BoneForPieceType(skeletonAsset, p.type);
            p.visible = 1;

            const glm::vec3 centerL = 0.5f * (sm.aabbMin + sm.aabbMax);
            const glm::vec3 extents = (sm.aabbMax - sm.aabbMin);

            glm::mat4 invFix = glm::rotate(glm::mat4(1.0f), glm::radians(-90.0f), glm::vec3(1, 0, 0));
            p.hitLocalCenter = glm::vec3(invFix * glm::vec4(centerL, 1.0f));

            glm::vec2 e2(extents.x, extents.y);
            p.hitRadius = 0.5f * glm::length(e2 * meshAsset->importScale);
            p.hitEnabled = 1;

            p.canDetach =
                (p.type == EnemyPieceType::Head) ||
                (p.type == EnemyPieceType::ArmL_Forearm) ||
                (p.type == EnemyPieceType::ArmR_Forearm) ||
                (p.type == EnemyPieceType::LegL_Calf) ||
                (p.type == EnemyPieceType::LegR_Calf);

            if (p.type == EnemyPieceType::Torso || p.type == EnemyPieceType::Hip)
                p.canDetach = 0;

            destrComp.pieces.push_back(p);
        }

        // AI / animation controller / state / vision
        NPCAIMovementComponent& movementComp = enemyNPCEntity.AddComponent<NPCAIMovementComponent>();
        movementComp.moveSpeed = prefab.moveSpeed;
        movementComp.radius = prefab.radius;

        NPCAIVisionComponent& visionComp = enemyNPCEntity.AddComponent<NPCAIVisionComponent>();
        visionComp.ViewAngle = prefab.viewAngleDeg;

        NpcAnimationControllerComponent& animCtrlComp = enemyNPCEntity.AddComponent<NpcAnimationControllerComponent>();
        NpcAIStateComponent& stateComp = enemyNPCEntity.AddComponent<NpcAIStateComponent>();
        NpcBodyStateComponent& pcBodyStateComponent = enemyNPCEntity.AddComponent<NpcBodyStateComponent>();

        if (prefab.addPatrol)
        {
            enemyNPCEntity.AddComponent<NpcAIPatrolComponent>();
        }

        // Resolve clips once; uses meshAsset (and can validate bone count)
        if (!animCtrlComp.clipsResolved)
        {
            SpawnUtils::ResolveZombieClips(animCtrlComp, *meshAsset);
            
        }

        return enemyNPCEntity;
    }

    float SpawnUtils::RandomFloat(float minV, float maxV)
    {
        static thread_local std::mt19937 rng{ std::random_device{}() };
        std::uniform_real_distribution<float> dist(minV, maxV);
        return dist(rng);
    }
}