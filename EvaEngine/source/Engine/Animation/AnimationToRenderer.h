#pragma once
#include "AnimationTypes.h"
#include "SpriteInstanceData.h"

namespace Engine {

    inline void FillSpriteInstanceFromClip(const AnimationClipRuntime& clip, uint32_t col,
        uint32_t row, SpriteInstanceData& inst)
    {
        const uint32_t idx = row * clip.grid.cols + col;
        const AnimationFrameUV uv = clip.uvTable[idx];

        inst.textureIndex = clip.grid.textureIndex;
        inst.uvMin16 = uv.uvMin16;
        inst.uvMax16 = uv.uvMax16;
        inst.frameSizePx = clip.frameSizePx;
        inst.pivotPx = clip.pivotPx;
        inst.pixelsPerUnit = clip.pixelsPerUnit;
    }

}
