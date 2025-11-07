#pragma once
#include "glm/glm.hpp"
#include <cstdint>

namespace Engine {

    // Keep this matching your GPU instance buffer layout
    struct SpriteInstanceData {
        glm::mat3x2 worldXform;   // 24 bytes
        uint32_t    textureIndex; // 4
        glm::u16vec2 uvMin16;     // 4
        glm::u16vec2 uvMax16;     // 4
        glm::u16vec2 frameSizePx; // 4
        glm::u16vec2 pivotPx;     // 4
        uint32_t    flags;        // 4 (bit0 flipX)
        float       pixelsPerUnit;// 4
        // total 48
    };

}
