#pragma once
#include <cstdint>

enum class LocomotionState : uint8_t
{
    Idle,
    Walk,
    Run
};

enum class MoveDirState : uint8_t
{
    Forward,
    Back,
    Left, 
    Right
};

struct CharacterAnimStateComponent
{
    LocomotionState locomotion = LocomotionState::Idle;
    MoveDirState moveDir = MoveDirState::Forward;

    uint8_t aiming = 0;
    uint8_t firing = 0;
    uint8_t reloading = 0;
};
