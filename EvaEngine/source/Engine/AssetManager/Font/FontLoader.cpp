#include "pch.h"

#define STB_TRUETYPE_IMPLEMENTATION
#include "stb_truetype.h"

#include "FontLoader.h"

#include <fstream>
#include <vector>
#include <algorithm>

#include <Engine/Platform/Vulkan/VulkanTexture.h>
#include <Engine/Core/Core.h>
#include "Engine/Platform/Vulkan/VulkanUtils.h"


namespace Engine
{
    static bool ReadFileBytes(const std::string& path, std::vector<uint8_t>& out)
    {
        std::ifstream f(path, std::ios::binary | std::ios::ate);
        if (!f)
            return false;

        std::streamsize sz = f.tellg();
        f.seekg(0, std::ios::beg);

        out.resize((size_t)sz);
        if (!f.read((char*)out.data(), sz))
            return false;

        return true;
    }

    Ref<Font> FontLoader::LoadTTF(const std::string& path, const FontLoadDesc& desc)
    {
        std::vector<uint8_t> ttf;
        if (!ReadFileBytes(path, ttf))
        {
            EE_CORE_ERROR("[FontLoader] Failed to read TTF: {}", path);
            return nullptr;
        }

        // Allocate atlas (R8)
        std::vector<uint8_t> atlas(desc.atlasWidth * desc.atlasHeight, 0);

        // stb bake uses this struct for packed glyphs
        std::vector<stbtt_bakedchar> baked(desc.charCount);

        const int res = stbtt_BakeFontBitmap(
            ttf.data(), 0,
            desc.pixelHeight,
            atlas.data(),
            (int)desc.atlasWidth, (int)desc.atlasHeight,
            desc.firstChar, desc.charCount,
            baked.data()
        );

        if (res <= 0)
        {
            EE_CORE_ERROR("[FontLoader] stbtt_BakeFontBitmap failed for {}", path);
            return nullptr;
        }

        // Build font object
        Ref<Font> font = std::make_shared<Font>();
        font->atlasWidth = desc.atlasWidth;
        font->atlasHeight = desc.atlasHeight;
        font->pixelHeight = desc.pixelHeight;

        // Get ascent/descent/lineGap (in font units -> pixels)
        stbtt_fontinfo info{};
        if (!stbtt_InitFont(&info, ttf.data(), stbtt_GetFontOffsetForIndex(ttf.data(), 0)))
        {
            EE_CORE_ERROR("[FontLoader] stbtt_InitFont failed for {}", path);
            return nullptr;
        }

        int ascent, descent, lineGap;
        stbtt_GetFontVMetrics(&info, &ascent, &descent, &lineGap);

        const float scale = stbtt_ScaleForPixelHeight(&info, desc.pixelHeight);
        font->ascentPx = ascent * scale;
        font->descentPx = descent * scale;  // note: descent is typically negative
        font->lineGapPx = lineGap * scale;

        // Fill glyphs (ASCII)
        for (int i = 0; i < desc.charCount; ++i)
        {
            const int ch = desc.firstChar + i;
            if (ch < 0 || ch >= 128)
                continue;

            // stbtt_bakedchar gives us:
            //  x0,y0,x1,y1 (pixel rect in atlas)
            //  xoff,yoff (top-left offset from baseline)
            //  xadvance (advance)
            const stbtt_bakedchar& bc = baked[i];

            Glyph& g = font->glyphs[ch];

            const int gw = (int)(bc.x1 - bc.x0);
            const int gh = (int)(bc.y1 - bc.y0);

            g.sizePx = { gw, gh };

            // bc.xoff, bc.yoff are offsets from the baseline to the glyph's top-left
            // Our convention in renderer: bearingPx = (left, top)
            // With y-down UI:
            //   top = -yoff
            g.bearingPx = { (int)std::round(bc.xoff), (int)std::round(-bc.yoff) };

            g.advancePx = bc.xadvance;

            // UVs
            const float invW = 1.0f / (float)desc.atlasWidth;
            const float invH = 1.0f / (float)desc.atlasHeight;

            g.uv0 = { bc.x0 * invW, bc.y0 * invH };
            g.uv1 = { bc.x1 * invW, bc.y1 * invH };
        }

        // Upload atlas to GPU
        // Prefer R8 if your VulkanTexture supports it. If not, expand to RGBA.
        if (!desc.forceRGBA)
        {
            // You need a VulkanTexture ctor or factory that accepts raw pixels and format R8.
            // If your VulkanTexture only supports RGBA, set forceRGBA=true.
            font->atlas = std::make_shared<VulkanTexture>(desc.atlasWidth, desc.atlasHeight, VK_FORMAT_R8_UNORM);
            
            VulkanUtils::TransitionImageLayout(font->atlas->GetImage(), VK_FORMAT_R8_UNORM,
                VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

            font->atlas->SetData(atlas.data(), (size_t)atlas.size());
                
        }
        else
        {
            std::vector<uint8_t> rgba(desc.atlasWidth * desc.atlasHeight * 4);
            for (uint32_t i = 0; i < desc.atlasWidth * desc.atlasHeight; ++i)
            {
                uint8_t a = atlas[i];
                rgba[i * 4 + 0] = 255;
                rgba[i * 4 + 1] = 255;
                rgba[i * 4 + 2] = 255;
                rgba[i * 4 + 3] = a;
            }
            font->atlas = std::make_shared<VulkanTexture>(desc.atlasWidth, desc.atlasHeight, VK_FORMAT_R8G8B8A8_UNORM);
            
            VulkanUtils::TransitionImageLayout(font->atlas->GetImage(), VK_FORMAT_R8G8B8A8_UNORM,
                VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

            
            font->atlas->SetData(atlas.data(), (size_t)atlas.size());

            
        }

        if (!font->atlas)
        {
            EE_CORE_ERROR("[FontLoader] Failed to create atlas texture for {}", path);
            return nullptr;
        }

        EE_CORE_INFO("[FontLoader] Loaded font '{}' size={} atlas={}x{}",
            path, desc.pixelHeight, desc.atlasWidth, desc.atlasHeight);

        return font;
    }
}
