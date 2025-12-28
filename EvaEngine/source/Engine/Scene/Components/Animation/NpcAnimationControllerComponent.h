#pragma once
#include <cstdint>
#include <Engine/Scene/Components/NPC/NpcAIComponent.h>
#include <Engine/Scene/Components/NPC/NpcAIStateComponent.h>


enum class NpcAnimRequest : uint8_t
{
    None,
    Hit,
    Attack,
    Death
};
// In your NPC animation controller component
struct NpcAnimationControllerComponent
{
    static constexpr uint32_t INVALID = 0xFFFFFFFFu;

    // Loop clips (ClipA)
    uint32_t clipIdle = INVALID;
    uint32_t clipWalk = INVALID;
    uint32_t clipRun = INVALID;
    uint32_t clipCrawl = INVALID;
    uint32_t clipDeath = INVALID;

    // Overlay clips (ClipB)
    uint32_t clipHit = INVALID;
    uint32_t clipHitGround = INVALID;
    uint32_t clipAttack = INVALID;
    uint32_t clipTrip = INVALID;
    uint32_t clipStandup = INVALID;

    // What loop we *want* on ClipA (so transitions can temporarily override ClipA)
    uint32_t currentLoopClip = INVALID;

    // Transition (full-body) one-shot that OWNS clipA (trip/fall/getup)
    uint32_t transitionClip = INVALID;
    float    transitionTimer = 0.0f;
    float    transitionDuration = 0.0f;

    // Overlay (hit/attack) one-shot that OWNS clipB
    uint32_t overlayClip = INVALID;
    float    actionTimer = 0.0f;  // overlay time remaining
    float    actionDuration = 0.0f;  // overlay total duration
    float    maxOverlayBlend = 0.6f; // how strong overlay can get (0..1)

    uint8_t  baseXFadeActive = 0;
    float    baseXFadeTimer = 0.0f;
    float    baseXFadeDuration = 0.0f;
    uint32_t baseNextClip = INVALID;


    // Requests from gameplay
    NpcAnimRequest request = NpcAnimRequest::None;

    // Optional (if you actually use it)
    AIState returnState = AIState::Idle;
};

