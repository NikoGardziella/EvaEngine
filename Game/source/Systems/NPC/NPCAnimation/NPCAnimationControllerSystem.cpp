#include "NPCAnimationControllerSystem.h"
#include <Engine/Scene/Scene.h>
#include <Engine/Debug/Instrumentor.h>
#include "Engine/Scene/Component.h"
#include <Engine/Scene/Components/Animation/NpcAnimationControllerComponent.h>
#include <Engine/AssetManager/AssetManager.h>
#include <Engine/Scene/Components/Render/3D/AnimatorComponent.h>
#include <Engine/Animation/3D/System/AnimUtils/AnimUtils.h>
#include <Engine/Scene/SceneUtils/SpawnUtils.h>
#include <Engine/Scene/Components/NPC/NpcAIStateComponent.h>
#include <Engine/Scene/Components/NPC/NpcBodyStateComponent.h>




void NPCAnimationControllerSystem::UpdateNPCAnimationControllerSystem(float dt, Engine::Scene* scene)
{
    EE_PROFILE_FUNCTION();

    auto& animReg = Engine::AssetManager::GetAnimationRegistry();
    const uint32_t INVALID = 0xFFFFFFFFu;

    scene->ForEach<NpcAIStateComponent, NpcBodyStateComponent, Engine::Animator3DComponent, NpcAnimationControllerComponent>(
        [&](Engine::Entity e, NpcAIStateComponent& npcStateComp, NpcBodyStateComponent& npcBodyComp,
            Engine::Animator3DComponent& animCOmp, NpcAnimationControllerComponent& ctrlCOmp)
        {
            
            if (npcStateComp.state == AIState::Dead)
            {
               ctrlCOmp.transitionTimer -= dt;
               return;
            }
            
            // 1) Base crossfade update (A->B base transition)
            //    While crossfading, clipB is occupied -> block overlay.
            if (ctrlCOmp.baseXFadeActive)
            {
                UpdateBaseCrossfade(animCOmp, ctrlCOmp, dt);

                if (ctrlCOmp.baseXFadeActive)
                {
                    if (ctrlCOmp.request == NpcAnimRequest::Hit || ctrlCOmp.request == NpcAnimRequest::Attack)
                        ctrlCOmp.request = NpcAnimRequest::None;
                }
            }

            // 2) Start BODY transition oneshot on ClipA (trip/fall/getup)
            //    Cancel base crossfade and overlay.
            if (npcBodyComp.transition != NpcTransition::None && ctrlCOmp.transitionTimer <= 0.0f)
            {
                const uint32_t tclip = TransitionClip(npcBodyComp, ctrlCOmp);
                npcBodyComp.transition = NpcTransition::None;

                if (tclip != INVALID)
                {
                    // cancel base crossfade
                    ctrlCOmp.baseXFadeActive = 0;
                    ctrlCOmp.baseXFadeTimer = 0.0f;
                    ctrlCOmp.baseXFadeDuration = 0.0f;
                    ctrlCOmp.baseNextClip = INVALID;

                    // cancel overlay
                    ctrlCOmp.actionTimer = 0.0f;
                    ctrlCOmp.actionDuration = 0.0f;
                    ctrlCOmp.overlayClip = INVALID;

                    animCOmp.clipB = INVALID;
                    animCOmp.timeB = 0.0f;
                    animCOmp.blend = 0.0f;

                    // start transition on A
                    ctrlCOmp.transitionClip = tclip;
                    ctrlCOmp.transitionDuration = GetClipDurationOrZero(animReg, tclip) / 1.0;
                    ctrlCOmp.transitionTimer = ctrlCOmp.transitionDuration;

                    Engine::AnimUtils::StartOneShotClipA(animCOmp, ctrlCOmp, animReg, tclip, npcStateComp.state);

                    // block queued hit/attack during transition
                    if (ctrlCOmp.request == NpcAnimRequest::Hit || ctrlCOmp.request == NpcAnimRequest::Attack)
                        ctrlCOmp.request = NpcAnimRequest::None;
                }
            }

            // 3) If transition active, update timer, block overlay, and on end
            //    crossfade to the correct base (crawl).
            if (ctrlCOmp.transitionTimer > 0.0f)
            {
                EE_PROFILE_FUNCTION();


                // Single decrement
                ctrlCOmp.transitionTimer -= dt;
                if (ctrlCOmp.transitionTimer < 0.0f)
                    ctrlCOmp.transitionTimer = 0.0f;

                // Block overlay requests while in body transition
                if (ctrlCOmp.request == NpcAnimRequest::Hit || ctrlCOmp.request == NpcAnimRequest::Attack)
                    ctrlCOmp.request = NpcAnimRequest::None;

                if (ctrlCOmp.transitionTimer <= 0.0f)
                {
                    // Transition finished
                    ctrlCOmp.transitionClip = INVALID;
                    ctrlCOmp.transitionDuration = 0.0f;

                    // If we ended in prone/crawl family, enforce Crawl locomotion
                    if (npcBodyComp.locomotion == NpcLocomotion::Prone || npcBodyComp.locomotion == NpcLocomotion::Crawl)
                        npcBodyComp.locomotion = NpcLocomotion::Crawl;

                    // Make sure no base blend is still considered "in progress"
                    ctrlCOmp.baseXFadeActive = 0;
                    ctrlCOmp.baseXFadeTimer = 0.0f;
                    ctrlCOmp.baseXFadeDuration = 0.0f;
                    ctrlCOmp.baseNextClip = INVALID;

                    // Also clear overlay if anything tried to sneak in
                    ctrlCOmp.actionTimer = 0.0f;
                    ctrlCOmp.actionDuration = 0.0f;
                    ctrlCOmp.overlayClip = INVALID;

                    // Start base crossfade to crawl (for non-death transitions)
                    const uint32_t crawlClip = ctrlCOmp.clipCrawl;
                    if (crawlClip != INVALID)
                    {
                        ctrlCOmp.currentLoopClip = crawlClip;
                        Engine::AnimUtils::StartBaseCrossfade(animCOmp, ctrlCOmp, crawlClip, 0.15f);
                    }
                    else
                    {
                        animCOmp.clipB = INVALID;
                        animCOmp.timeB = 0.0f;
                        animCOmp.blend = 0.0f;
                    }
                }

                return;
            }



            // 4) Compute desired base loop
            uint32_t desiredBase = ctrlCOmp.clipIdle;

            if (npcBodyComp.locomotion == NpcLocomotion::Prone || npcBodyComp.locomotion == NpcLocomotion::Crawl)
            {
                desiredBase = ctrlCOmp.clipCrawl;
            }
            else if (npcBodyComp.locomotion == NpcLocomotion::Walk)
            {
                switch (npcStateComp.state)
                {
                case AIState::Idle:  desiredBase = ctrlCOmp.clipIdle; break;
                case AIState::Patrol:desiredBase = ctrlCOmp.clipWalk; break;
                case AIState::ChaseLOS:
                case AIState::MoveToLastKnown:
                case AIState::Attack:desiredBase = ctrlCOmp.clipRun;  break;
                default:             desiredBase = ctrlCOmp.clipIdle; break;
                }
            }

            // 5) If desired base changed, start a base crossfade
            if (!ctrlCOmp.baseXFadeActive && desiredBase != INVALID && desiredBase != ctrlCOmp.currentLoopClip)
            {
                animCOmp.loopAclip = true;

                ctrlCOmp.currentLoopClip = desiredBase;
                Engine::AnimUtils::StartBaseCrossfade(animCOmp, ctrlCOmp, desiredBase, 0.15f);
            }

            // 6) Overlay (only when NOT base-crossfading)
            if (!ctrlCOmp.baseXFadeActive)
            {
                if (ctrlCOmp.actionTimer > 0.0f)
                {
                    UpdateOneShotB(animCOmp, ctrlCOmp, dt);
                    if (ctrlCOmp.actionTimer > 0.0f)
                        return;
                }

                if (ctrlCOmp.request != NpcAnimRequest::None)
                {
                    const NpcAnimRequest req = ctrlCOmp.request;
                    ctrlCOmp.request = NpcAnimRequest::None;

                    if (req == NpcAnimRequest::Hit)
                    {
                        uint32_t clip = ctrlCOmp.clipHit;

                        if (npcBodyComp.locomotion == NpcLocomotion::Crawl ||
                            npcBodyComp.locomotion == NpcLocomotion::Prone)
                        {
                            clip = ctrlCOmp.clipHitGround;
                        }

                        Engine::AnimUtils::StartOneShotClipB(animCOmp, ctrlCOmp, animReg, clip, npcStateComp.state, 0.5f);
                        return;
                    }

                    if (req == NpcAnimRequest::Attack)
                    {
                        const bool canAttack = (npcBodyComp.canAttack != 0);
                        const bool canDoAttackAnim = canAttack &&
                            (npcBodyComp.locomotion != NpcLocomotion::Crawl) &&
                            (npcBodyComp.locomotion != NpcLocomotion::Prone);

                        if (canDoAttackAnim)
                        {
                            Engine::AnimUtils::StartOneShotClipB(animCOmp, ctrlCOmp, animReg, ctrlCOmp.clipAttack, AIState::ChaseLOS, 0.5f);
                        }

                        return;
                    }

                    if (req == NpcAnimRequest::Death)
                    {

                        ctrlCOmp.transitionDuration = GetClipDurationOrZero(animReg, ctrlCOmp.clipDeath);
                        ctrlCOmp.transitionTimer = ctrlCOmp.transitionDuration;
                        animCOmp.loopAclip = false;
                        if (npcBodyComp.locomotion != NpcLocomotion::Crawl)
                        {
                            Engine::AnimUtils::StartOneShotClipA(animCOmp, ctrlCOmp, animReg, ctrlCOmp.clipDeath, npcStateComp.state);
                        }
                        return;
                    }
                }
            }
            else
            {
                // Prevent queued overlay during base blend
                if (ctrlCOmp.request == NpcAnimRequest::Hit || ctrlCOmp.request == NpcAnimRequest::Attack)
                    ctrlCOmp.request = NpcAnimRequest::None;
            }

            // 7) Optional: AI Attack -> request once
            if (!ctrlCOmp.baseXFadeActive && npcStateComp.state == AIState::Attack && ctrlCOmp.actionTimer <= 0.0f)
            {
                if (npcBodyComp.canAttack && npcBodyComp.locomotion != NpcLocomotion::Crawl && npcBodyComp.locomotion != NpcLocomotion::Prone)
                    ctrlCOmp.request = NpcAnimRequest::Attack;
            }
        });
}


void NPCAnimationControllerSystem::UpdateOneShotB(Engine::Animator3DComponent& anim, NpcAnimationControllerComponent& ctrl, float dt)
{
    const uint32_t INVALID = NpcAnimationControllerComponent::INVALID;

    if (ctrl.actionTimer <= 0.0f)
    {
        anim.clipB = INVALID;
        anim.timeB = 0.0f;
        anim.blend = 0.0f;
        ctrl.actionDuration = 0.0f;
        ctrl.overlayClip = INVALID;
        return;
    }

    ctrl.actionTimer -= dt;
    if (ctrl.actionTimer < 0.0f) ctrl.actionTimer = 0.0f;

    anim.timeB += dt * ((anim.playbackSpeed > 1e-4f) ? anim.playbackSpeed : 1.0f);

    const float dur = glm::max(ctrl.actionDuration, 1e-4f);
    const float progress = 1.0f - (ctrl.actionTimer / dur); // 0..1

    const float fadeIn = 0.08f;
    const float fadeOut = 0.12f;

    float wIn = (fadeIn > 0.0f) ? (progress * dur) / fadeIn : 1.0f;
    float wOut = (fadeOut > 0.0f) ? ((1.0f - progress) * dur) / fadeOut : 1.0f;

    float w = glm::min(glm::clamp(wIn, 0.0f, 1.0f), glm::clamp(wOut, 0.0f, 1.0f));
    w = Smooth01(w);

    anim.blend = w * ctrl.maxOverlayBlend;

    if (ctrl.actionTimer <= 0.0f)
    {
        anim.clipB = INVALID;
        anim.timeB = 0.0f;
        anim.blend = 0.0f;
        ctrl.actionDuration = 0.0f;
        ctrl.overlayClip = INVALID;
    }
}


uint32_t NPCAnimationControllerSystem::TransitionClip(const NpcBodyStateComponent& body, const NpcAnimationControllerComponent& ctrl)
{
    switch (body.transition)
    {
    case NpcTransition::FallToProne: return ctrl.clipTrip;
    case NpcTransition::GetUpToWalk: return ctrl.clipStandup;
    default: return 0xFFFFFFFFu;
    }
}

float  NPCAnimationControllerSystem::GetClipDurationOrZero(const Engine::AnimationRegistry& animReg, uint32_t clip)
{
    const uint32_t INVALID = 0xFFFFFFFFu;
    if (clip == INVALID) return 0.0f;
    const auto& c = animReg.Get(clip);
    return c.duration > 0.0f ? c.duration : 0.0f;
}

float NPCAnimationControllerSystem::Smooth01(float x)
{
    x = glm::clamp(x, 0.0f, 1.0f);
    return x * x * (3.0f - 2.0f * x);
}

void NPCAnimationControllerSystem::UpdateBaseCrossfade(Engine::Animator3DComponent& anim,  NpcAnimationControllerComponent& ctrl,
    float dt)
{
    if (!ctrl.baseXFadeActive) return;

    ctrl.baseXFadeTimer -= dt;
    if (ctrl.baseXFadeTimer < 0.0f) ctrl.baseXFadeTimer = 0.0f;

    float t = 1.0f - (ctrl.baseXFadeTimer / ctrl.baseXFadeDuration); // 0->1
    float w = Smooth01(t);

    anim.blend = w; // for base blend, full 0..1

    if (ctrl.baseXFadeTimer <= 0.0f)
    {
        // Commit: new base becomes clipA
        anim.clipA = ctrl.baseNextClip;
        anim.timeA = anim.timeB; // keep whatever timeB reached during blend

        // Clear clipB
        anim.clipB = NpcAnimationControllerComponent::INVALID;
        anim.timeB = 0.0f;
        anim.blend = 0.0f;

        ctrl.baseXFadeActive = 0;
        ctrl.baseNextClip = NpcAnimationControllerComponent::INVALID;
        ctrl.baseXFadeDuration = 0.0f;
    }
}