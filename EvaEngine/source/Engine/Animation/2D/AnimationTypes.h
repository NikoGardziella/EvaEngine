#pragma once
#include "glm/glm.hpp"
#include <string>
#include <vector>
#include <cstdint>

namespace Engine {

    // Quantized UV rect for a frame
    struct AnimationFrameUV {
        glm::u16vec2 uvMin16{ 0,0 };
        glm::u16vec2 uvMax16{ 0,0 };
    };

    // Grid clip description
    struct AnimationClipGrid {
        uint16_t cols = 0;     // frames per direction
        uint16_t rows = 0;     // 8 for Dir8
        uint16_t cellW = 0;    // pixels
        uint16_t cellH = 0;    // pixels
        float    fps = 12.0f;
        uint8_t  loop = 1;     // bool, packed
        uint32_t textureIndex = 0xFFFFFFFFu; // bindless index from streaming
    };
    struct AnimationEvent {
        uint16_t frame = 0;
        uint16_t row = 0;       // direction row or 0 for any
        uint32_t nameHash = 0;    // HashUtils::Hash32("footstep")
    };
    // Runtime clip asset (precomputed UVs)
    struct Animation2DClipRuntime {
        std::string name;
        AnimationClipGrid grid;
        std::vector<AnimationFrameUV> uvTable; // rows * cols

        glm::u16vec2 frameSizePx{ 0,0 };   // {cellW, cellH}
        glm::u16vec2 pivotPx{ 0,0 };       // bottom center pivot
        float        pixelsPerUnit = 64.0f;
        uint32_t     texWidth = 0;
        uint32_t     texHeight = 0;

        // NEW: map logical Dir8 -> sheet row index (0..rows-1)
        uint8_t dirToRow[8] = { 0,1,2,3,4,5,6,7 };
        std::unordered_map<uint32_t, std::vector<std::string>> events;

    };

    // Events authored in YAML (optional)
    

    // Per-entity animator state (simple)
    struct AnimatorState {
        uint16_t frame = 0;       // current column
        float    time = 0.0f;    // time in current frame
        float    speed = 1.0f;    // playback scalar
    };

}