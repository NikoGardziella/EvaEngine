#pragma once
#include <cstdint>
#include <vector>
#include "glm/glm.hpp"
#include <Engine/Core/Config.h>

namespace Engine {


    struct Animator3DComponent
    {
        // Base layer (you already have)
        AnimClipId clipA = INVALID_CLIP;
        AnimClipId clipB = INVALID_CLIP;
        float timeA = 0.0f;
        float timeB = 0.0f;
        float blend = 0.0f; // base crossfade 0..1
        float playbackSpeed = 1.0f;
        uint8_t useRootMotion = 1;
        bool loopAclip = false;

        // Overlay layer (upper body)
        AnimClipId overlayClip = INVALID_CLIP;
        float overlayTime = 0.0f;
        float overlayWeight = 0.0f;   // 0..1 (drive this when aiming/shooting)
        bool overlayLoop = false;
        uint8_t overlayRestart = 0;

        // Mask: per-bone [0..1], size = boneCount
        std::vector<float> overlayMask; // 0 for legs/hips, 1 for spine+arms+head

        std::vector<glm::mat4> boneModel;
    };

}