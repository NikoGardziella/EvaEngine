#pragma once
#include <vulkan/vulkan_core.h>

namespace Engine {

	enum class ImageLayoutState
	{
		Undefined,
		ShaderRead,
		ColorAttachment,
		Present,
		TransferDst,
		TransferSrc,
		ShaderReadOnlyOptimal,
		General
	};

	class VulkanTrackedImageUtils
	{
	public:


		static VkImageLayout ToVkImageLayout(ImageLayoutState layout)
		{
			switch (layout)
			{
			case ImageLayoutState::Undefined: return VK_IMAGE_LAYOUT_UNDEFINED;
			case ImageLayoutState::ShaderRead: return VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			case ImageLayoutState::ColorAttachment: return VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
			case ImageLayoutState::Present: return VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
			case ImageLayoutState::TransferDst: return VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
			case ImageLayoutState::TransferSrc: return VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
			case ImageLayoutState::ShaderReadOnlyOptimal: return VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			case ImageLayoutState::General: return VK_IMAGE_LAYOUT_GENERAL;
			}
			return VK_IMAGE_LAYOUT_UNDEFINED; // fallback
		}
	};

	
    struct VulkanTracked
    {
        VkImage image = VK_NULL_HANDLE;
        VkImageView view = VK_NULL_HANDLE;
        VkFormat format = VK_FORMAT_UNDEFINED;
        uint32_t width = 0;
        uint32_t height = 0;
        uint32_t mipLevels = 1;
        uint32_t layers = 1;
        ImageLayoutState currentLayoutState = ImageLayoutState::Undefined;
        VkImageLayout currentLayout = VK_IMAGE_LAYOUT_UNDEFINED;

        void Transition(
            VkCommandBuffer commandBuffer,
            ImageLayoutState newLayoutState
        ) {
            

            // Map ImageLayoutState (your enum) to real VkImageLayout
            VkImageLayout oldLayoutVk = VulkanTrackedImageUtils::ToVkImageLayout(currentLayoutState);
            VkImageLayout newLayoutVk = VulkanTrackedImageUtils::ToVkImageLayout(newLayoutState);

            VkAccessFlags srcAccessMask = 0;
            VkAccessFlags dstAccessMask = 0;
            VkPipelineStageFlags srcStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
            VkPipelineStageFlags dstStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;

            if (currentLayoutState == newLayoutState)
            {
                return;
            }

            // Setup source access and stage masks based on current layout
            switch (currentLayoutState)
            {
            case ImageLayoutState::Undefined:
                srcAccessMask = 0;
                srcStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
                break;
            case ImageLayoutState::ColorAttachment:
                srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT;
                srcStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
                break;
            case ImageLayoutState::Present:
                srcAccessMask = VK_ACCESS_MEMORY_READ_BIT;
                srcStage = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
                break;
            case ImageLayoutState::ShaderRead:
            case ImageLayoutState::ShaderReadOnlyOptimal:
                srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
                srcStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
                break;
            case ImageLayoutState::TransferSrc:
                srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
                srcStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
                break;
            case ImageLayoutState::TransferDst:
                srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
                srcStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
                break;
            case ImageLayoutState::General:
                srcAccessMask = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT;
                srcStage = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT | VK_PIPELINE_STAGE_TRANSFER_BIT;
                break;
            }

            // Setup destination access and stage masks based on new layout
            switch (newLayoutState)
            {
            case ImageLayoutState::ColorAttachment:
                dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
                dstStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
                break;
            case ImageLayoutState::Present:
                dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
                dstStage = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
                break;
            case ImageLayoutState::ShaderRead:
            case ImageLayoutState::ShaderReadOnlyOptimal:
                dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
                dstStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
                break;
            case ImageLayoutState::TransferSrc:
                dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
                dstStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
                break;
            case ImageLayoutState::TransferDst:
                dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
                dstStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
                break;
            case ImageLayoutState::General:
                dstAccessMask = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT;
                dstStage = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT | VK_PIPELINE_STAGE_TRANSFER_BIT;
                break;
            case ImageLayoutState::Undefined:
                // Rare to transition *to* Undefined, but let's handle safely
                dstAccessMask = 0;
                dstStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
                break;
            }

            VkImageMemoryBarrier barrier{};
            barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
            barrier.oldLayout = oldLayoutVk;
            barrier.newLayout = newLayoutVk;
            barrier.srcAccessMask = srcAccessMask;
            barrier.dstAccessMask = dstAccessMask;
            barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            barrier.image = image;
            barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            barrier.subresourceRange.baseMipLevel = 0;
            barrier.subresourceRange.levelCount = mipLevels;
            barrier.subresourceRange.baseArrayLayer = 0;
            barrier.subresourceRange.layerCount = layers;

            vkCmdPipelineBarrier(
                commandBuffer,
                srcStage,
                dstStage,
                0,
                0, nullptr,
                0, nullptr,
                1, &barrier
            );

            // Track the updated layout
            currentLayoutState = newLayoutState;
            currentLayout = newLayoutVk;
        }
    




	};


	

}

