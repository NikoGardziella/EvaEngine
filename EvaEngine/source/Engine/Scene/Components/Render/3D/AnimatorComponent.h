#pragma once
#include <cstdint>

namespace Engine {


    struct Animator3DComponent {
        // runtime pose state
        AnimClipId clipA = INVALID_CLIP;
        AnimClipId clipB = INVALID_CLIP;
        float timeA = 0.0f;
        float timeB = 0.0f;
        float blend = 0.0f;          // 0..1
        uint32_t stateId = 0;
        uint32_t transitionId = 0xFFFFFFFFu;
        float playbackSpeed = 1.0f;
        uint8_t useRootMotion = 1;

        //TOdo update the animations less when they are further
        uint8_t  updateRate = 1;
        uint8_t  frameCounter = 0; 
    };
}