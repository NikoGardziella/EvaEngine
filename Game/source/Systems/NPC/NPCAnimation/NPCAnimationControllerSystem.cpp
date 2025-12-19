#include "NPCAnimationControllerSystem.h"
#include <Engine/Scene/Scene.h>
#include <Engine/Debug/Instrumentor.h>
#include "Engine/Scene/Component.h"
#include <Engine/Scene/Components/Animation/NpcAnimationControllerComponent.h>
#include <Engine/AssetManager/AssetManager.h>
#include <Engine/Scene/Components/Render/3D/AnimatorComponent.h>
#include <Engine/Animation/3D/System/AnimUtils/AnimUtils.h>
#include <Engine/Scene/SceneUtils/SpawnUtils.h>

void NPCAnimationControllerSystem::UpdateNPCAnimationControllerSystem(float dt, Engine::Scene* scene)
{
    EE_PROFILE_FUNCTION();

    auto& animReg = Engine::AssetManager::GetAnimationRegistry();

    scene->ForEach<NPCAIMovementComponent, Engine::Animator3DComponent, NpcAnimationControllerComponent>(
        [&](Engine::Entity e, NPCAIMovementComponent& ai, Engine::Animator3DComponent& anim, NpcAnimationControllerComponent& ctrl)
        {
            if (!ctrl.clipsResolved)
                Engine::SpawnUtils::ResolveZombieClips(ctrl);

            // 0) Always ensure clipA is a valid loop for the current state (even during one-shot)
            switch (ai.CurrentState)
            {
            case AIState::Idle:        Engine::AnimUtils::SetLoopClip(anim, ctrl, ctrl.clipIdle);  break;
            case AIState::Patrol:      Engine::AnimUtils::SetLoopClip(anim, ctrl, ctrl.clipWalk);  break;
            case AIState::MoveToTarget:
            case AIState::ChaseLOS:   Engine::AnimUtils::SetLoopClip(anim, ctrl, ctrl.clipRun);   break;

            case AIState::Death:       Engine::AnimUtils::SetLoopClip(anim, ctrl, ctrl.clipDeath); break;

                // If you have crawl state:
                // case AIState::Crawl:    SetLoopClip(anim, ctrl, ctrl.clipCrawl); break;

            default:                   Engine::AnimUtils::SetLoopClip(anim, ctrl, ctrl.clipIdle); break;
            }

            // 1) If a one-shot is active, keep updating it (and don't restart it)
            if (ctrl.actionTimer > 0.0f)
            {
                // death overrides everything
                if (ctrl.currentClip == ctrl.clipDeath)
                    return;

                UpdateOneShot(anim, ctrl, dt);
                if (ctrl.actionTimer > 0.0f)
                    return; // keep overlay running this tick
                // else it just ended -> fallthrough
            }

            // 2) Consume request (starts a one-shot overlay)
            if (ctrl.request != NpcAnimRequest::None)
            {
                const NpcAnimRequest req = ctrl.request;
                ctrl.request = NpcAnimRequest::None;

                if (req == NpcAnimRequest::Hit)
                {
                    EE_CORE_INFO("hit req");
                    Engine::AnimUtils::StartOneShot(anim, ctrl, animReg, ctrl.clipHit, ai.CurrentState);
                    return;
                }
                if (req == NpcAnimRequest::Attack)
                {
                    Engine::AnimUtils::StartOneShot(anim, ctrl, animReg, ctrl.clipAttack, AIState::ChaseLOS);
                    return;
                }
                if (req == NpcAnimRequest::Death)
                {
                    // death as loop clipA, no overlay needed
                    ai.CurrentState = AIState::Death;
                    Engine::AnimUtils::SetLoopClip(anim, ctrl, ctrl.clipDeath);
                    return;
                }
            }

            // 3) (Optional) AIState::Attack can trigger request once
            // DO NOT spam it every frame:
            if (ai.CurrentState == AIState::Attack && ctrl.actionTimer <= 0.0f)
            {
                ctrl.request = NpcAnimRequest::Attack;
            }
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
