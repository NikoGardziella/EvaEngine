#include "pch.h"
#include "SpawnUtils.h"
#include <Engine/AssetManager/AssetManager.h>
#include <Engine/Animation/3D/AnimationRegistry.h>
#include <Engine/Scene/Components/Render/3D/AnimatorComponent.h>

namespace Engine {

    uint32_t SpawnUtils::FindClipId(const char* name)
    {
        auto& animReg = Engine::AssetManager::GetAnimationRegistry();
        if (const Engine::AnimationClip* c = animReg.FindAnimationClip(name))
            return c->id;
        return 0xFFFFFFFFu;
    }



    void SpawnUtils::ResolveZombieClips(NpcAnimationControllerComponent& animControllerComp)
    {
        animControllerComp.clipAgonize = FindClipId("zombieAgonizing");   // if your registry uses this name
        animControllerComp.clipCrawl = FindClipId("zombieAnimCrawl");
        animControllerComp.clipIdle = FindClipId("zombieAnimIdle");
        animControllerComp.clipWalk = FindClipId("zombieAnimWalk");
        animControllerComp.clipHit = FindClipId("zombieAnimHeadHit");
        animControllerComp.clipDeath = FindClipId("zombieAnimDeath");
        animControllerComp.clipAttack = FindClipId("zombieAnimStrike");
        animControllerComp.clipRun = FindClipId("zombieAnimRun");

        animControllerComp.clipsResolved = true;
    }


}