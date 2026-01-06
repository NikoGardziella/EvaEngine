#pragma once
#include <cstdint>
#include <Engine/Animation/2D/AnimationTypes.h>
#include <Engine/Animation/2D/AnimationEnums.h>

namespace Engine {

    struct Animation2DComponent {
        uint32_t clipId = 0;
        Dir8     direction = Dir8::S;

        float    aimRadians = 0.0f;    // still used for target angle
        float    facingRadians = 0.0f; // smoothed, what sprite actually faces
        float    turnSpeed = glm::radians(720.0f); // deg/s -> tweak
        uint8_t  dirMode = 0;
        uint8_t  paused = 0;
        uint16_t _pad = 0;
    };

    struct AnimatorStateComponent {
        AnimatorState state;       // frame/time/speed
    };

    
}
