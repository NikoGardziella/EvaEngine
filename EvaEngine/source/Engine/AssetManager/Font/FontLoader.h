#pragma once

#include <string>
#include <Engine/UI/Font.h>
#include <Engine/Core/Core.h>

namespace Engine
{
    struct FontLoadDesc
    {
        float pixelHeight = 32.0f;     // font size in pixels
        uint32_t atlasWidth = 1024;
        uint32_t atlasHeight = 1024;

        // ASCII range
        int firstChar = 32;
        int charCount = 95;            // 32..126 inclusive

        // If your VulkanTexture doesn't support R8, set to true to expand to RGBA.
        bool forceRGBA = false;
    };

    class FontLoader
    {
    public:
        // path = .ttf file on disk
        static Ref<Font> LoadTTF(const std::string& path, const FontLoadDesc& desc);
    };
}
