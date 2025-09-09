#include "pch.h"

#include "TextureStreamingUtils.h"
#include <Engine/Scene/Components/Render/TileComponent.h>
#include <Engine/Core/Core.h>
#include "Engine/AssetManager/AssetManager.h"
#include <Engine/Scene/Components/Render/RoofRenderComponent.h>
#include "Engine/Platform/Vulkan/VulkanUtils.h"
#include "Engine/Scene/Entity.h"
#include <Engine/Scene/Components/Render/DynamicObjectRenderComp.h>
#include <Engine/Core/Log.h>
#include <Engine/Scene/Component.h>

namespace Engine {

    bool TextureStreamingUtils::BakeDynamicObjectIfNeeded(Scene* scene, Entity entity)
    {
        EE_PROFILE_FUNCTION();

        auto& reg = scene->GetRegistry();
        if (!reg.all_of<TileComponent, TransformComponent>(entity)) return false;

        const TileComponent& tc = reg.get<TileComponent>(entity);
        if (tc.tiles.empty()) return false;

        // world-units per pixel (same as your uploader/compute)
        const float pxWorld = float(TILE_SIZE) / float(TILE_PIXEL_WIDTH);

        // Collect dynamic tiles and compute AABB in *local pixel space* (relative to entity).
        struct SrcTile
        {
            glm::ivec2 basePx;              // bottom-center of the tile in local pixels
            std::vector<uint8_t> color;
            std::vector<uint8_t> props;
            int w = 0, h = 0;
        };

        std::vector<SrcTile> srcs;
        srcs.reserve(tc.tiles.size());

        glm::ivec2 minPx(std::numeric_limits<int>::max());
        glm::ivec2 maxPx(-std::numeric_limits<int>::max());
        bool hasDyn = false;

        for (const auto& t : tc.tiles)
        {
            if (t.Category != eTileCategory::dynamicObjects) continue;
            hasDyn = true;

            // IMPORTANT: match your uploader, which floors world?pixels
            // (Your static uploader did: groundPx = floor(world * CELL);)
            glm::ivec2 basePx = glm::ivec2(glm::floor(t.position / pxWorld));

            SrcTile S;
            S.basePx = basePx;

            if (!AssetManager::ExtractPixelsFromTilePallette(t, S.color, S.props, S.w, S.h))
                continue;

            // AABB contribution using bottom-center pivot (identical logic to uploader)
            const int halfW = S.w / 2;                    // integer floor
            const glm::ivec2 tileMin = { basePx.x - halfW, basePx.y };
            const glm::ivec2 tileMax = { tileMin.x + S.w, tileMin.y + S.h }; // exclusive

            minPx = glm::min(minPx, tileMin);
            maxPx = glm::max(maxPx, tileMax);

            srcs.push_back(std::move(S));
        }

        if (!hasDyn || srcs.empty()) return false;

        const glm::ivec2 texSize = maxPx - minPx; // exclusive bounds -> size
        if (texSize.x <= 0 || texSize.y <= 0) return false;

        // Compose to final atlases (bottom-origin rows — same convention as your pipeline)
        std::vector<uint8_t> finalColor(size_t(texSize.x) * size_t(texSize.y) * 4, 0);
        std::vector<uint8_t> finalProps(size_t(texSize.x) * size_t(texSize.y) * 4, 0);

        for (const auto& S : srcs)
        {
            const int halfW = S.w / 2;
            const glm::ivec2 dstOrigin = (S.basePx - minPx) + glm::ivec2(-halfW, 0);

            for (int y = 0; y < S.h; ++y)
            {
                const int dstY = dstOrigin.y + y;
                if ((unsigned)dstY >= (unsigned)texSize.y) continue;

                const size_t srcRow = size_t(y) * size_t(S.w) * 4;
                const size_t dstRow = size_t(dstY) * size_t(texSize.x) * 4;

                for (int x = 0; x < S.w; ++x)
                {
                    const int dstX = dstOrigin.x + x;
                    if ((unsigned)dstX >= (unsigned)texSize.x) continue;

                    const size_t si = srcRow + size_t(x) * 4;
                    const size_t di = dstRow + size_t(dstX) * 4;

                    // COLOR: straight alpha-over
                    uint8_t& dR = finalColor[di + 0];
                    uint8_t& dG = finalColor[di + 1];
                    uint8_t& dB = finalColor[di + 2];
                    uint8_t& dA = finalColor[di + 3];

                    const uint8_t sR = S.color.empty() ? 0 : S.color[si + 0];
                    const uint8_t sG = S.color.empty() ? 0 : S.color[si + 1];
                    const uint8_t sB = S.color.empty() ? 0 : S.color[si + 2];
                    const uint8_t sA = S.color.empty() ? 0 : S.color[si + 3];

                    (void)TextureStreamingUtils::AlphaOver(sR, sG, sB, sA, dR, dG, dB, dA);

                    // PROPS: merge w/ color coverage
                    uint8_t& dPr = finalProps[di + 0];
                    uint8_t& dPg = finalProps[di + 1];
                    uint8_t& dPb = finalProps[di + 2];
                    uint8_t& dPa = finalProps[di + 3];

                    const uint8_t sPr = S.props.empty() ? 0 : S.props[si + 0];
                    const uint8_t sPg = S.props.empty() ? 0 : S.props[si + 1];
                    const uint8_t sPb = S.props.empty() ? 0 : S.props[si + 2];
                    const uint8_t sPa = S.props.empty() ? 0 : S.props[si + 3];

                    const uint8_t sAcov = S.color.empty() ? 255 : S.color[si + 3];
                    TextureStreamingUtils::MergePropertiesPixel(sPr, sPg, sPb, sPa, sAcov,
                        dPr, dPg, dPb, dPa);
                }
            }
        }

        Ref<VulkanTexture> colorTex = std::make_shared<VulkanTexture>(
            texSize.x, texSize.y, VK_FORMAT_R8G8B8A8_UNORM);

        VulkanUtils::TransitionImageLayout(colorTex->GetImage(), VK_FORMAT_R8G8B8A8_UNORM,
            VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
        colorTex->SetData(finalColor.data(), finalColor.size());
        
        Ref<VulkanTexture> propsTex = std::make_shared<VulkanTexture>(
            texSize.x, texSize.y, VK_FORMAT_R8G8B8A8_UINT);

        VulkanUtils::TransitionImageLayout(propsTex->GetImage(), VK_FORMAT_R8G8B8A8_UINT,
            VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
        propsTex->SetData(finalProps.data(), finalProps.size());
       
        auto& dyn = reg.emplace_or_replace<DynamicObjectRenderComp>(entity);
        dyn.Texture = colorTex;
        dyn.PropertiesTexture = propsTex;
        dyn.IsLoaded = true;


        dyn.OriginBLWorld = glm::vec2(minPx) * pxWorld;
        dyn.WorldSize = glm::vec2(texSize) * pxWorld;
        EE_CORE_INFO("Creating tile at OriginBLWorld {}, {}", dyn.OriginBLWorld.x, dyn.OriginBLWorld.y);
        return true;
    }




    bool TextureStreamingUtils::BakeRoofTextureIfNeeded(Scene* scene, Entity entity)
    {
        EE_PROFILE_FUNCTION();
        if (!scene->GetRegistry().all_of<TileComponent, TransformComponent>(entity))
            return false;

        auto& tcomp = scene->GetRegistry().get<TileComponent>(entity);
        auto& xf = scene->GetRegistry().get<TransformComponent>(entity);

        constexpr int TILE_SIZE_PX = TILE_PIXEL_WIDTH;

        glm::ivec2 minPos(INT_MAX);
        glm::ivec2 maxPos(INT_MIN);
        bool hasRoofTile = false;

        // Find bounding box of roof tiles in local tile coords
        for (const auto& tile : tcomp.tiles)
        {
            if (tile.Category != eTileCategory::Roofs)
                continue;

            hasRoofTile = true;
            glm::ivec2 tilePos = glm::ivec2(tile.position);
            minPos = glm::min(minPos, tilePos);
            maxPos = glm::max(maxPos, tilePos);
        }

        if (!hasRoofTile)
            return false;

        // Calculate texture size in pixels (width, height)
        glm::ivec2 textureSize = (maxPos - minPos + glm::ivec2(1)) * TILE_SIZE_PX;

        std::vector<uint8_t> finalPixels(textureSize.x * textureSize.y * 4, 0);

        // Copy tile pixels into finalPixels buffer
        for (const auto& tile : tcomp.tiles)
        {
            if (!tile.IsRoof)
                continue;

            glm::ivec2 localOffsetPixels = (glm::ivec2(tile.position) - minPos) * TILE_SIZE_PX ;

            std::vector<uint8_t> pixelData;
            int width, height;
            if (!AssetManager::ExtractPixelsFromTilePallette(tile, pixelData, width, height))
                continue;

            for (int y = 0; y < height; ++y)
            {
                for (int x = 0; x < width; ++x)
                {
                    int srcIdx = (y * width + x) * 4;
                    int dstX = localOffsetPixels.x + x;
                    int dstY = localOffsetPixels.y + y;
                    int dstIdx = (dstY * textureSize.x + dstX) * 4;

                    if (dstX >= 0 && dstX < textureSize.x && dstY >= 0 && dstY < textureSize.y)
                    {
                        for (int c = 0; c < 4; ++c)
                            finalPixels[dstIdx + c] = pixelData[srcIdx + c];
                    }
                }
            }
        }

        // Create Vulkan texture (width, height)
        Ref<VulkanTexture> combinedTexture = std::make_shared<VulkanTexture>(textureSize.x, textureSize.y);
        VulkanUtils::TransitionImageLayout(
            combinedTexture->GetImage(),
            VK_FORMAT_R8G8B8A8_UNORM,
            VK_IMAGE_LAYOUT_UNDEFINED,
            VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
        );
        combinedTexture->SetData(finalPixels.data(), textureSize.x * textureSize.y * 4);

        // Store RoofRenderComponent with positive LocalOffset in world units
        RoofRenderComponent& roofRenderComp = scene->GetRegistry().emplace_or_replace<RoofRenderComponent>(entity, RoofRenderComponent{});
        roofRenderComp.IsLoaded = true;
        roofRenderComp.Texture = combinedTexture;


        glm::vec2 localCenter = glm::vec2(minPos + maxPos) * 0.5f + glm::vec2(0.5f);
        glm::vec2 centerOffsetWorld = localCenter * float(TILE_SIZE);
        roofRenderComp.LocalOffset = glm::vec3(centerOffsetWorld, 0.0f);

        EE_CORE_INFO("Entity pos: {}, {} | Roof LocalOffset: {}, {}", xf.Translation.x, xf.Translation.y, roofRenderComp.LocalOffset.x, roofRenderComp.LocalOffset.y);

        return true;
    }


    bool TextureStreamingUtils::BakeVehicleTextureIfNeeded(Scene* scene, Entity entity)
    {
        EE_PROFILE_FUNCTION();

        if (!scene->GetRegistry().all_of<TileComponent, TransformComponent>(entity))
            return false;

        auto& tcomp = scene->GetRegistry().get<TileComponent>(entity);
        auto& xf = scene->GetRegistry().get<TransformComponent>(entity);

        constexpr int TILE_SIZE_PX = TILE_PIXEL_WIDTH;

        glm::ivec2 minPos(INT_MAX);
        glm::ivec2 maxPos(INT_MIN);
        bool hasVehicleTile = false;

        for (const auto& tile : tcomp.tiles)
        {
            if (tile.Category != eTileCategory::Vehicles)
                continue;

            hasVehicleTile = true;
            glm::ivec2 tilePos = glm::ivec2(tile.position);
            minPos = glm::min(minPos, tilePos);
            maxPos = glm::max(maxPos, tilePos);
        }

        if (!hasVehicleTile)
            return false;

        glm::ivec2 textureSize = (maxPos - minPos + glm::ivec2(1)) * TILE_SIZE_PX;
        std::vector<uint8_t> finalPixels(textureSize.x * textureSize.y * 4, 0);

        for (const auto& tile : tcomp.tiles)
        {
            if (tile.Category != eTileCategory::Vehicles)
                continue;

            glm::ivec2 localOffsetPixels = (glm::ivec2(tile.position) - minPos) * TILE_SIZE_PX;

            std::vector<uint8_t> pixelData;
            int width, height;
            if (!AssetManager::ExtractPixelsFromTilePallette(tile, pixelData, width, height))
                continue;

            for (int y = 0; y < height; ++y)
            {
                for (int x = 0; x < width; ++x)
                {
                    int srcIdx = (y * width + x) * 4;
                    int dstX = localOffsetPixels.x + x;
                    int dstY = localOffsetPixels.y + y;
                    int dstIdx = (dstY * textureSize.x + dstX) * 4;

                    if (dstX >= 0 && dstX < textureSize.x && dstY >= 0 && dstY < textureSize.y)
                    {
                        for (int c = 0; c < 4; ++c)
                            finalPixels[dstIdx + c] = pixelData[srcIdx + c];
                    }
                }
            }
        }

        Ref<VulkanTexture> vehicleTexture = std::make_shared<VulkanTexture>(textureSize.x, textureSize.y);
        VulkanUtils::TransitionImageLayout(
            vehicleTexture->GetImage(),
            VK_FORMAT_R8G8B8A8_UNORM,
            VK_IMAGE_LAYOUT_UNDEFINED,
            VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
        );
        vehicleTexture->SetData(finalPixels.data(), textureSize.x * textureSize.y * 4);

        SpriteRendererComponent& vehicleRender = scene->GetRegistry().emplace_or_replace<SpriteRendererComponent>(entity);
        //vehicleRender.IsLoaded = true;
        vehicleRender.Texture = vehicleTexture;
        vehicleRender.Color = glm::vec4(1);
        glm::vec2 localCenter = glm::vec2(minPos + maxPos) * 0.5f + glm::vec2(0.5f);
        glm::vec2 centerOffsetWorld = localCenter * float(TILE_SIZE);
        //vehicleRender.LocalOffset = glm::vec3(centerOffsetWorld, 0.0f);

     
        return true;
    }


   

    // signed floor division that matches GLSL-style tiling
    int TextureStreamingUtils::FloorDiv(int a, int b) 
    {
        int q = a / b, r = a % b;
        if ((r != 0) && ((r > 0) != (b > 0))) --q;
        return q;
    }

    // pack/unpack A-channel as [category: hi 4 bits | flags: lo 4 bits]
    void TextureStreamingUtils::UnpackCategoryFlags(uint8_t a, uint8_t& category, uint8_t& flags)
    {
        category = static_cast<uint8_t>(a >> 4);
        flags = static_cast<uint8_t>(a & 0x0F);
    }
    uint8_t TextureStreamingUtils::PackCategoryFlags(uint8_t category, uint8_t flags)
    {
        return static_cast<uint8_t>((category << 4) | (flags & 0x0F));
    }


    // straight alpha “src over dst” for **non-premultiplied** RGBA8
    bool TextureStreamingUtils::AlphaOver(uint8_t sR, uint8_t sG, uint8_t sB, uint8_t sA,
        uint8_t& dR, uint8_t& dG, uint8_t& dB, uint8_t& dA)
    {
        if (sA == 0) return false; // nothing changed

        const float sa = sA / 255.0f;
        const float da = dA / 255.0f;

        const float sr = sR / 255.0f, sg = sG / 255.0f, sb = sB / 255.0f;
        const float dr = dR / 255.0f, dg = dG / 255.0f, db = dB / 255.0f;

        const float outA = sa + da * (1.0f - sa);

        float outR = dr, outG = dg, outB = db;
        if (outA > 0.0f) {
            const float oneMinusSa = (1.0f - sa);
            outR = (sr * sa + dr * da * oneMinusSa) / outA;
            outG = (sg * sa + dg * da * oneMinusSa) / outA;
            outB = (sb * sa + db * da * oneMinusSa) / outA;
        }

        dR = static_cast<uint8_t>(std::clamp(outR * 255.0f, 0.0f, 255.0f));
        dG = static_cast<uint8_t>(std::clamp(outG * 255.0f, 0.0f, 255.0f));
        dB = static_cast<uint8_t>(std::clamp(outB * 255.0f, 0.0f, 255.0f));
        dA = static_cast<uint8_t>(std::clamp(outA * 255.0f, 0.0f, 255.0f));

        return true; // changed
    }

    inline uint8_t GetCategory(uint8_t a) { return uint8_t(a & 0xF0); }
    inline uint8_t GetFlags(uint8_t a) { return uint8_t(a & 0x0F); }
    inline uint8_t MakeA(uint8_t categoryHi, uint8_t flagsLo) { return uint8_t((categoryHi & 0xF0) | (flagsLo & 0x0F)); }

    // merge one properties pixel: R=max health, G=max height, B=max mask,
    // A = [category: prefer non-zero source | flags: OR]
    bool TextureStreamingUtils::MergePropertiesPixel(uint8_t sPr, uint8_t sPg, uint8_t sPb, uint8_t sPa, uint8_t sAcov,
        uint8_t& dPr, uint8_t& dPg, uint8_t& dPb, uint8_t& dPa)
    {
        // If the source doesn’t cover this pixel at all, don’t change anything.
        if (sAcov == 0)
        {
            return false;
        }

        bool changed = false;

        // Health: favor "more solid"
        uint8_t newPr = std::max(dPr, sPr);
        if (newPr != dPr) { dPr = newPr; changed = true; }

        // Height (rows-above-pivot): favor taller
        uint8_t newPg = std::max(dPg, sPg);
        if (newPg != dPg) { dPg = newPg; changed = true; }

        // Mask/scratch (e.g., foot band): favor broader/stronger
        uint8_t newPb = std::max(dPb, sPb);
        if (newPb != dPb) { dPb = newPb; changed = true; }

        // ---- A channel: [category<<4 | flags] ----
        const uint8_t dstCat = GetCategory(dPa);
        const uint8_t dstFlg = GetFlags(dPa);
        const uint8_t srcCat = GetCategory(sPa);
        const uint8_t srcFlg = GetFlags(sPa);

        // Category is *selected* by covered source (top-most), but don’t overwrite with 0-category.
        const uint8_t outCat = (srcCat != 0 ? srcCat : dstCat);

        // Flags accumulate with OR.
        const uint8_t outFlg = uint8_t(dstFlg | srcFlg);

        const uint8_t outA = MakeA(outCat, outFlg);
        if (outA != dPa) { dPa = outA; changed = true; }

        return changed;
    }


   




}