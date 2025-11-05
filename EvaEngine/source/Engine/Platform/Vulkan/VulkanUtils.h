#pragma once
#include "VulkanContext.h"
#include <string>
#include "VulkanGraphicsPipeline.h"
#include <Engine/Renderer/VulkanRenderer2D.h>

namespace Engine {

	namespace VulkanUtils
	{
    
	    VulkanContext::QueueFamilyIndices FindQueueFamilies(VkPhysicalDevice device, VkSurfaceKHR surface);
		VulkanContext::SwapChainSupportDetails QuerySwapChainSupport(VkPhysicalDevice device, VkSurfaceKHR surface);

        void CreateImage(uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage,
            VkMemoryPropertyFlags properties, VkImage& image, VkDeviceMemory& imageMemory);

       void TransitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout);

        void TransitionImageLayout(VkCommandBuffer commandBuffer, VkImage image, VkFormat format, VkImageLayout oldLayout,
            VkImageLayout newLayout);

        void CopyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height);

       void CopyBufferToImage(VkCommandBuffer commandBuffer, VkBuffer buffer, VkImage image, uint32_t width, uint32_t height);

       const std::string LayoutToString(VkImageLayout layout);

       void CopyBufferToImage(StorageImage image, VkBuffer buffer);

       EffectPushConstants MakeDefaultEffectsState();

       void BarrierColorReadOnlyToGeneral(VkCommandBuffer cmd, VkImage colorArray, uint32_t layer);
       void BarrierPropsComputeToFragVisibility(VkCommandBuffer cmd, VkImage propsArray, uint32_t layer);
       void BarrierColorGeneralToReadOnly(VkCommandBuffer cmd, VkImage colorArray, uint32_t layer);
       VkDeviceSize AlignUp(VkDeviceSize v, VkDeviceSize a);
       uint32_t TileToFXIndex(const glm::vec2& tileOriginW, const glm::vec2& fxGridTopLeftW, float pixelSizeW, int tilePxW, int tilePxH, bool worldYDown);
	}

	

	
}


