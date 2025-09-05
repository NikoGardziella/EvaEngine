#include "pch.h"

#include "TextureStreamingUtils.h"
#include <Engine/Scene/Components/Render/TileComponent.h>
#include <Engine/Core/Core.h>
#include "Engine/AssetManager/AssetManager.h"
#include <Engine/Scene/Components/Render/RoofRenderComponent.h>
#include "Engine/Platform/Vulkan/VulkanUtils.h"
#include "Engine/Scene/Entity.h"

namespace Engine {


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

    // merge one properties pixel: R=max health, G=max height, B=max mask,
    // A = [category: prefer non-zero source | flags: OR]
    bool TextureStreamingUtils::MergePropertiesPixel(uint8_t sPr, uint8_t sPg, uint8_t sPb, uint8_t sPa, uint8_t sCoverageA,
        uint8_t& dPr, uint8_t& dPg, uint8_t& dPb, uint8_t& dPa)
    {
        if (sCoverageA == 0) return false; // respect color coverage gate when available

        uint8_t newPr = std::max(dPr, sPr);
        uint8_t newPg = std::max(dPg, sPg);
        uint8_t newPb = std::max(dPb, sPb);

        uint8_t dCat, dFl; UnpackCategoryFlags(dPa, dCat, dFl);
        uint8_t sCat, sFl; UnpackCategoryFlags(sPa, sCat, sFl);

        // category policy: prefer non-zero source; keep dest otherwise
        // (swap with a priority rule if you need strict precedence)
        const uint8_t outCat = (sCat != 0u) ? sCat : dCat;
        const uint8_t outFl = static_cast<uint8_t>(dFl | sFl);
        const uint8_t newPa = PackCategoryFlags(outCat, outFl);

        const bool changed = (newPr != dPr) | (newPg != dPg) | (newPb != dPb) | (newPa != dPa);

        dPr = newPr; dPg = newPg; dPb = newPb; dPa = newPa;
        return changed;
    }

   




}