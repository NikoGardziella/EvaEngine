#pragma once

#include <cstdint>


enum class NpcTransition : uint8_t
{
    None = 0,
    FallToProne,    // legs removed -> fall
    GetUpToWalk,    // prone -> stand
    GetUpToCrawl    // prone -> crawl-ready (optional)
};

enum class NpcLocomotion : uint8_t
{
    Walk = 0,
    Crawl,
    Prone,
    Dead
};

enum NpcCapability : uint32_t
{
    Cap_None = 0,
    Cap_Move = 1u << 0,
    Cap_AttackMelee = 1u << 1,
    Cap_AttackBite = 1u << 2,
    Cap_Sense = 1u << 3,
};

static inline bool NpcHasCap(uint32_t caps, NpcCapability c)
{
    return (caps & (uint32_t)c) != 0u;
}

struct NpcBodyStateComponent
{
    NpcLocomotion locomotion = NpcLocomotion::Walk;
    uint32_t caps = Cap_Move | Cap_AttackMelee | Cap_Sense;

    // outputs that other systems can use directly
    float moveSpeedMul = 1.0f;
    float attackRangeMul = 1.0f;
    uint8_t canAttack = 1;

    // transition “event” (set for one frame, consumed by animation controller)
    NpcTransition transition = NpcTransition::None;

    // small memory so we can detect changes
    uint8_t prevHadAnyLeg = 1;     // runtime
    uint8_t prevIsProne = 0;     // runtime
};
