#include "CharacterAnimationControllerSystem.h"
#include <Engine/Scene/Components/Render/3D/AnimatorComponent.h>
#include <Engine/Core/Core.h>
#include <glm/common.hpp>
#include <Engine/Scene/Scene.h>
#include <Engine/Scene/Components/Animation/PlayerAnimation/CharacterAnimStateComponent.h>
#include <Engine/Scene/Components/Animation/PlayerAnimation/CharacterAnimSetComponent.h>
#include <Engine.h>
#include <Engine/AssetManager/AssetManager.h>



static void SetBaseLoop(Engine::Animator3DComponent& an, AnimClipId clip)
{
    if (an.clipA == clip && an.clipB == INVALID_CLIP)
        return;

    // Start crossfade: A <- current pose, B <- new clip
    an.clipA = an.clipA == INVALID_CLIP ? clip : an.clipA;
    an.clipB = clip;
    an.timeB = 0.0f;
    an.blend = 0.0f;
    an.loopAclip = true;
}

static void StepBaseCrossfade(Engine::Animator3DComponent& an, float dt, float fadeSpeed)
{
    if (an.clipB == INVALID_CLIP)
        return;

    an.blend = glm::clamp(an.blend + fadeSpeed * dt, 0.0f, 1.0f);

    if (an.blend >= 1.0f)
    {
        // Commit B -> A, clear B
        an.clipA = an.clipB;
        an.timeA = an.timeB;

        an.clipB = INVALID_CLIP;
        an.timeB = 0.0f;
        an.blend = 0.0f;
        an.loopAclip = true;
    }
}

void CharacterAnimationControllerSystem::UpdateCharacterAnimationControllerSystem(float deltatime, Engine::Scene* scene)
{
    EE_PROFILE_FUNCTION();

    scene->ForEach<CharacterAnimStateComponent, CharacterAnimSetComponent, Engine::Animator3DComponent>(
        [&](Engine::Entity /*e*/, CharacterAnimStateComponent& st, CharacterAnimSetComponent& set, Engine::Animator3DComponent& an)
        {
            // 1) Pick base target clip
            AnimClipId desiredBase = set.idle;
            if (st.locomotion == LocomotionState::Run) desiredBase = set.run;
            if (st.locomotion == LocomotionState::Walk) desiredBase = set.walk;
            if (st.locomotion == LocomotionState::Idle) desiredBase = set.idle;

            // 2) Ensure base is playing desired clip (crossfade)
            if (an.clipA == INVALID_CLIP)
            {
                an.clipA = desiredBase;
                an.timeA = 0.0f;
                an.loopAclip = true;
                an.clipB = INVALID_CLIP;
                an.timeB = 0.0f;
                an.blend = 0.0f;
            }
            else if (an.clipA != desiredBase && an.clipB != desiredBase)
            {
                an.clipB = desiredBase;
                an.timeB = 0.0f;
                an.blend = 0.0f; // start fade
            }

            StepBaseCrossfade(an, deltatime, set.baseFade);


            //auto& animReg = Engine::AssetManager::GetAnimationRegistry();

            // 3) Overlay setup (aim + fire)
            // Inputs/state booleans for this frame:
            const bool aiming = (st.aiming != 0);
            const bool fireHeld = (st.firing != 0); // make this "held", not pulse

            if (fireHeld)
            {
                // Keep playing shoot while held
                if (an.overlayClip != set.fireRifle)
                {
                    an.overlayClip = set.fireRifle;
                    an.overlayTime = 0.0f;     // optional: restart when you begin holding
                }

                an.overlayLoop = true;         // loop for automatic fire look
                an.overlayWeight = 1.0f;
            }
            else if (aiming)
            {
                // Aim stance while aiming (if you have one)
                if (set.aimIdle != INVALID_CLIP)
                {
                    an.overlayClip = set.idle;
                    an.overlayLoop = true;
                    an.overlayWeight = 1.0f;
                }
                else
                {
                    // Temporary: freeze first frame of shoot as an "aim stance"
                    an.overlayClip = set.fireRifle;
                    an.overlayLoop = false;
                    an.overlayWeight = 1.0f;
                    an.overlayTime = 0.0f;
                }
            }
            else
            {
                // No overlay
                an.overlayWeight = 0.0f;
                an.overlayClip = INVALID_CLIP;
                an.overlayLoop = false;
                an.overlayTime = 0.0f;
            }

            // consume pulse so it doesn't retrigger
        });
}

