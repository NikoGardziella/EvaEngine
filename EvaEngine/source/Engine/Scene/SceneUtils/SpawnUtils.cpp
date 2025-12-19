#include "pch.h"
#include "SpawnUtils.h"
#include <Engine/AssetManager/AssetManager.h>
#include <Engine/Animation/3D/AnimationRegistry.h>
#include <Engine/Scene/Components/Render/3D/AnimatorComponent.h>
#include <Engine/Animation/3D/SkeletonRegistry.h>

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
        animControllerComp.clipRun = FindClipIdChecked("zombieAnimRun", skeletonId, meshBoneCount);

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
    }



}