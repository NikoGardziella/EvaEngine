#pragma once
#include "Engine/Core/Core.h"

#include "glm/glm.hpp"
#include <array>

namespace Engine {

    class VulkanTexture;
    struct Glyph
    {
        glm::ivec2 sizePx;      // bitmap w,h
        glm::ivec2 bearingPx;   // left, top
        float advancePx;        // advance in pixels
        glm::vec2 uv0;          // atlas uv min
        glm::vec2 uv1;          // atlas uv max
    };

    struct Font
    {
        Ref<VulkanTexture> atlas;     // R8 or RGBA atlas
        uint32_t atlasWidth = 0;
        uint32_t atlasHeight = 0;

        float pixelHeight = 0.0f;
        float ascentPx = 0.0f;
        float descentPx = 0.0f;
        float lineGapPx = 0.0f;

        // ASCII for now (0..127). We'll fill 32..126.
        std::array<Glyph, 128> glyphs{};
    };
}
