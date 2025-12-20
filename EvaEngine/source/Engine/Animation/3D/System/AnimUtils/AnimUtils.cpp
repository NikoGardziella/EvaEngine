#include "pch.h"
#include "AnimUtils.h"
#include <Engine/Scene/Components/Render/3D/AnimatorComponent.h>
#include "Engine/Scene/Entity.h"
#include <Engine/AssetManager/AssetManager.h>
#include <Engine/Scene/Components/NPC/NpcAIComponent.h>
#include <Engine/Scene/Components/Animation/NpcAnimationControllerComponent.h>
#include <Engine/Scene/Components/NPC/NpcAIStateComponent.h>
#include <Engine/Animation/3D/AnimationRegistry.h>

namespace Engine {

    void AnimUtils::SetLoopClip(Animator3DComponent& anim, NpcAnimationControllerComponent& ctrl, uint32_t clipId)
    {
        if (clipId == 0xFFFFFFFFu) return;
        if (ctrl.currentClip == clipId) return;

        anim.clipA = clipId;
        anim.timeA = 0.0f;
        anim.blend = 0.0f;

        ctrl.currentClip = clipId;
    }


    float AnimUtils::FindClipDuration(uint32_t clipId)
    {
        if (clipId == 0xFFFFFFFFu) return 0.0f;
        auto& animReg = Engine::AssetManager::GetAnimationRegistry();
        return animReg.Get(clipId).duration; // adjust if your API differs
    }

    void AnimUtils::StartOneShot(Engine::Animator3DComponent& anim,  NpcAnimationControllerComponent& ctrl,
        const Engine::AnimationRegistry& animReg, uint32_t clipBId, AIState returnState)
    {
        const uint32_t INVALID = 0xFFFFFFFFu;
        if (clipBId == INVALID) return;

        const Engine::AnimationClip& clipB = animReg.Get(clipBId);

        ctrl.returnState = returnState;

        // Start overlay
        anim.clipB = clipBId;
        anim.timeB = 0.0f;
        anim.blend = 0.5f;

        float speed = (anim.playbackSpeed != 0.0f) ? anim.playbackSpeed : 1.0f;
        ctrl.actionDuration = clipB.duration / speed;
        ctrl.actionTimer = ctrl.actionDuration;
    }


}