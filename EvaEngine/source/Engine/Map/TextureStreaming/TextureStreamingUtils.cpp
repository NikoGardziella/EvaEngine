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






}