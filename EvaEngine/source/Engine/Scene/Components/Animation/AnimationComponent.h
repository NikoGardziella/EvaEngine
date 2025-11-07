#pragma once
#include <cstdint>
#include <Engine/Animation/AnimationTypes.h>
#include <Engine/Animation/AnimationEnums.h>

namespace Engine {

    struct AnimationComponent {
        uint32_t clipId = 0;       // registry id or hash for "player/run"
        Dir8     direction = Dir8::S;
        float    aimRadians = 0.0f; // if using aim based selection
        uint8_t  dirMode = 0;       // 0: Velocity, 1: Aim, 2: Manual
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
