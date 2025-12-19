#pragma once
#include <cstdint>
#include <Engine/Scene/Components/NPC/NpcAIComponent.h>


enum class NpcAnimRequest : uint8_t
{
    None,
    Hit,
    Attack,
    Death
};

struct NpcAnimationControllerComponent
{
    uint32_t clipIdle = 0xFFFFFFFFu;
    uint32_t clipWalk = 0xFFFFFFFFu;
    uint32_t clipRun = 0xFFFFFFFFu;
    uint32_t clipCrawl = 0xFFFFFFFFu;
    uint32_t clipAttack = 0xFFFFFFFFu;
    uint32_t clipHit = 0xFFFFFFFFu;
    uint32_t clipDeath = 0xFFFFFFFFu;
    uint32_t clipAgonize = 0xFFFFFFFFu;

    bool clipsResolved = false;

    // one-shot playback
    float actionTimer = 0.0f;
    float actionDuration = 0.0f;
    AIState returnState = AIState::Idle;

    // one-shot request (set by other systems)
    NpcAnimRequest request = NpcAnimRequest::None;

    // optional: avoid re-setting same clip every frame
    uint32_t currentClip = 0xFFFFFFFFu;
};
