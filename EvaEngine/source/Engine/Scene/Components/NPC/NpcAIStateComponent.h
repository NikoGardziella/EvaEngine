#pragma once
#include <cstdint>
#include "glm/glm.hpp"

enum class AIState : uint8_t
{
    Idle = 0,
    Patrol,
    ChaseLOS,
    MoveToLastKnown,
    Attack,
};

struct NpcAIStateComponent
{
    AIState state = AIState::Idle;

    // Timers/state bookkeeping
    float idleTimer = 0.0f;
    float idleDuration = 0.8f;

    // Patrol
    uint32_t patrolIndex = 0;

    // Repath throttle for MoveToLastKnown
    float repathTimer = 0.0f;
    float repathInterval = 0.25f;

    // Last-known target memory (you can also read this from vision if you want)
    glm::vec3 lastKnownPos = glm::vec3(0.0f);
    uint8_t   hasLastKnown = 0;

    // Output “orders” for movement system
    uint8_t wantsMove = 0;
    glm::vec2 moveGoal2D = glm::vec2(0.0f); // goal in XY
    uint8_t wantsPath = 0;                  // movement system should pathfind if 1

    // Output “orders” for animation controller (optional)
    uint8_t wantsAttack = 0;
};
