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

    struct AnimationEventBufferComponent {
        // Simple ring buffer of fired event name hashes per frame
        static constexpr int MaxEvents = 16;
        uint32_t events[MaxEvents]{};
        uint8_t  count = 0;
        uint8_t  _pad[3]{};
        inline void Clear() { count = 0; }
        inline void Push(uint32_t h) { if (count < MaxEvents) events[count++] = h; }
    };

}
