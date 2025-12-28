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
        const uint32_t INVALID = NpcAnimationControllerComponent::INVALID;
        if (clipId == INVALID) return;

        // If a transition is currently owning ClipA, do NOT fight it
        if (ctrl.transitionTimer > 0.0f)
            return;

        if (ctrl.currentLoopClip == clipId && anim.clipA == clipId)
            return;

        anim.clipA = clipId;
        anim.timeA = 0.0f;
        anim.loopAclip = true;
        ctrl.currentLoopClip = clipId;
    }
    


    void AnimUtils::StartBaseCrossfade(Engine::Animator3DComponent& anim,
        NpcAnimationControllerComponent& ctrl,
        uint32_t nextClip,
        float durationSeconds)
    {
        const uint32_t INVALID = NpcAnimationControllerComponent::INVALID;
        if (nextClip == INVALID) return;

        // If already on it, nothing to do
        if (anim.clipA == nextClip && ctrl.baseXFadeActive == 0)
            return;

        // Cancel overlay during base crossfade (optional but recommended)
        ctrl.actionTimer = 0.0f;
        anim.clipB = INVALID;
        anim.timeB = 0.0f;
        anim.blend = 0.0f;

        ctrl.baseXFadeActive = 1;
        ctrl.baseXFadeDuration = glm::max(durationSeconds, 1e-3f);
        ctrl.baseXFadeTimer = ctrl.baseXFadeDuration;
        ctrl.baseNextClip = nextClip;

        // Put "next base" on ClipB
        anim.clipB = nextClip;
        anim.timeB = 0.0f; // or keep normalized time (optional)
        anim.blend = 0.0f;
    }





    float AnimUtils::FindClipDuration(uint32_t clipId)
    {
        if (clipId == 0xFFFFFFFFu) return 0.0f;
        auto& animReg = Engine::AssetManager::GetAnimationRegistry();
        return animReg.Get(clipId).duration; // adjust if your API differs
    }



       

  

        void Engine::AnimUtils::StartOneShotClipB(Engine::Animator3DComponent& anim,
            NpcAnimationControllerComponent& ctrl,
            const Engine::AnimationRegistry& animReg,
            uint32_t clipBId,
            AIState returnState,
            float maxBlend /*= 0.6f*/)
        {
            const uint32_t INVALID = 0xFFFFFFFFu;
            if (clipBId == INVALID) return;

            // Block overlay while full-body transition owns clipA (trip/fall/getup)
            if (ctrl.transitionTimer > 0.0f)
                return;

            // Optional: if overlay already active, ignore (prevents hit-spam restarting)
            if (ctrl.actionTimer > 0.0f)
                return;

            const Engine::AnimationClip& clipB = animReg.Get(clipBId);
            if (clipB.duration <= 1e-4f)
                return;

            ctrl.returnState = returnState;

            // Start overlay on ClipB
            anim.clipB = clipBId;
            anim.timeB = 0.0f;

            // Let UpdateOneShotB drive blend (fade in/out). Start at 0 for nice fade-in.
            anim.blend = 0.0f;

            const float speed = (anim.playbackSpeed > 1e-4f) ? anim.playbackSpeed : 1.0f;

            ctrl.actionDuration = clipB.duration / speed;
            ctrl.actionTimer = ctrl.actionDuration;
            ctrl.overlayClip = clipBId;
            ctrl.maxOverlayBlend = glm::clamp(maxBlend, 0.0f, 1.0f);
        }


    void AnimUtils::StartOneShotClipA(Engine::Animator3DComponent& anim, NpcAnimationControllerComponent& ctrl,
        Engine::AnimationRegistry& animReg, uint32_t clipAId, AIState returnState)
    {
        const uint32_t INVALID = 0xFFFFFFFFu;
        if (clipAId == INVALID) return;

        const Engine::AnimationClip& clipA = animReg.Get(clipAId);

        ctrl.returnState = returnState;
        
        float speed = (anim.playbackSpeed != 0.0f) ? anim.playbackSpeed : 1.0f;
       
  

        // Start overlay
        anim.clipA = clipAId;
        anim.timeB = 0.0f;
        anim.timeA = 0.0f;
        anim.blend = 0.5f;
        anim.loopAclip = false;

        ctrl.actionDuration = clipA.duration / speed;
        ctrl.actionTimer = ctrl.actionDuration;
    }

}