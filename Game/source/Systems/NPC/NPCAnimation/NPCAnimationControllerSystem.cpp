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

static uint32_t TransitionClip(const NpcBodyStateComponent& body, const NpcAnimationControllerComponent& ctrl)
{
    switch (body.transition)
    {
    case NpcTransition::FallToProne: return ctrl.clipTrip;
    case NpcTransition::GetUpToWalk: return ctrl.clipStandup;
    default: return 0xFFFFFFFFu;
    }
}

void NPCAnimationControllerSystem::UpdateNPCAnimationControllerSystem(float dt, Engine::Scene* scene)
{
    EE_PROFILE_FUNCTION();
    auto& animReg = Engine::AssetManager::GetAnimationRegistry();

    scene->ForEach<NpcAIStateComponent, NpcBodyStateComponent, Engine::Animator3DComponent, NpcAnimationControllerComponent>(
        [&](Engine::Entity e, NpcAIStateComponent& ai, NpcBodyStateComponent& body,
            Engine::Animator3DComponent& anim, NpcAnimationControllerComponent& ctrl)
        {
            // 0) death hard override
            if (body.locomotion == NpcLocomotion::Dead)
            {
                Engine::AnimUtils::SetLoopClip(anim, ctrl, ctrl.clipDeath);
                ctrl.request = NpcAnimRequest::None;
                body.transition = NpcTransition::None;
                return;
            }

            // 1) choose base loop for current locomotion + AI state
            if (body.locomotion == NpcLocomotion::Dead)
                Engine::AnimUtils::SetLoopClip(anim, ctrl, ctrl.clipDeath);
            else if (body.locomotion == NpcLocomotion::Prone)
                Engine::AnimUtils::SetLoopClip(anim, ctrl, ctrl.clipCrawl);  // or reuse crawl loop if you only have one
            else if (body.locomotion == NpcLocomotion::Crawl)
                Engine::AnimUtils::SetLoopClip(anim, ctrl, ctrl.clipCrawl);
               
            
            else
            {
                switch (ai.state)
                {
                case AIState::Idle:  Engine::AnimUtils::SetLoopClip(anim, ctrl, ctrl.clipIdle); break;
                case AIState::Patrol:Engine::AnimUtils::SetLoopClip(anim, ctrl, ctrl.clipWalk); break;
                case AIState::ChaseLOS:
                case AIState::MoveToLastKnown:
                case AIState::Attack:Engine::AnimUtils::SetLoopClip(anim, ctrl, ctrl.clipRun);  break;
                default:             Engine::AnimUtils::SetLoopClip(anim, ctrl, ctrl.clipIdle); break;
                }
            }

            // 2) if a one-shot is active, keep updating it
            if (ctrl.actionTimer > 0.0f)
            {
                UpdateOneShot(anim, ctrl, dt);
                if (ctrl.actionTimer > 0.0f)
                    return; // still playing
                // finished -> continue
            }

            // 3) consume BODY transition first (fall/get-up)
            if (body.transition != NpcTransition::None)
            {
                const uint32_t clip = TransitionClip(body, ctrl);
                body.transition = NpcTransition::None; // consume so it won't re-trigger

                if (clip != 0xFFFFFFFFu)
                {
                    anim.playbackSpeed = 1.5f;
                    // During transitions, usually you don’t want to be interrupted by hit/attack
                    Engine::AnimUtils::StartOneShotClipB(anim, ctrl, animReg, clip, ai.state);

                    // If this was a fall, you might want to automatically enter Crawl after the one-shot:
                    // you already set locomotion = Prone in BodyStateSystem; optionally switch to Crawl once finished
                    // (If you want that, do it when the oneshot ends; see below.)
                    return;
                }
            }

            // 4) normal requests (hit/attack/death)
            if (ctrl.request != NpcAnimRequest::None)
            {
                const NpcAnimRequest req = ctrl.request;
                ctrl.request = NpcAnimRequest::None;

                if (req == NpcAnimRequest::Hit)
                {
                    Engine::AnimUtils::StartOneShotClipB(anim, ctrl, animReg, ctrl.clipHit, ai.state);
                    return;
                }
                if (req == NpcAnimRequest::Attack)
                {
                    const bool canAttack = (body.canAttack != 0);
                    const bool canDoAttackAnim = canAttack && (body.locomotion != NpcLocomotion::Crawl) && (body.locomotion != NpcLocomotion::Prone);
                    if (canDoAttackAnim)
                        Engine::AnimUtils::StartOneShotClipB(anim, ctrl, animReg, ctrl.clipAttack, AIState::ChaseLOS);
                    return;
                }
                if (req == NpcAnimRequest::Death)
                {
                    body.locomotion = NpcLocomotion::Dead;
                    Engine::AnimUtils::SetLoopClip(anim, ctrl, ctrl.clipDeath);
                    return;
                }
            }

            // 5) AI Attack -> request once (optional)
            if (ai.state == AIState::Attack && ctrl.actionTimer <= 0.0f)
            {
                if (body.canAttack && body.locomotion != NpcLocomotion::Crawl && body.locomotion != NpcLocomotion::Prone)
                    ctrl.request = NpcAnimRequest::Attack;
            }

            // Optional: after fall one-shot finishes, auto go to Crawl loop:
            // You can do this in UpdateOneShot end detection instead (best place).
        });
}


void NPCAnimationControllerSystem::UpdateOneShot(Engine::Animator3DComponent& anim, NpcAnimationControllerComponent& ctrl, float dt)
{
    const uint32_t INVALID = 0xFFFFFFFFu;
   

    if (ctrl.actionTimer <= 0.0f)
    {
        // Ensure cleared
        anim.clipB = INVALID;
        anim.timeB = 0.0f;
        anim.blend = 0.0f;
        return;
    }

    ctrl.actionTimer -= dt;
    if (ctrl.actionTimer < 0.0f) ctrl.actionTimer = 0.0f;

    // Drive timeB forward (no looping)
    anim.timeB += dt * anim.playbackSpeed;

    // Fade in/out (hardcoded but good defaults)
    const float fadeIn = 0.08f;
    const float fadeOut = 0.12f;

    float dur = (ctrl.actionDuration > 1e-4f) ? ctrl.actionDuration : 1e-4f;
    float t = 1.0f - (ctrl.actionTimer / dur); // 0->1 progress

    float w = 1.0f;
    if (fadeIn > 0.0f)  w = glm::clamp((t * dur) / fadeIn, 0.0f, 1.0f);
    if (fadeOut > 0.0f) w = glm::min(w, glm::clamp(((1.0f - t) * dur) / fadeOut, 0.0f, 1.0f));

    //  smoothing
    w = w * w * (3.0f - 2.0f * w);

    const float maxOverlay = 0.6f;
    anim.blend = glm::clamp(w, 0.0f, 1.0f) * maxOverlay;

    if (ctrl.actionTimer <= 0.0f)
    {
        anim.clipB = INVALID;
        anim.timeB = 0.0f;
        anim.blend = 0.0f;
        ctrl.actionDuration = 0.0f;

    }
}
