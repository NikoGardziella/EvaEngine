#pragma once
#include <Engine/Renderer/Utils/DeltaBitReader.h>
#include <Engine/Core/Log.h>
#include <Engine/Platform/Vulkan/VulkanUtils.h>


namespace Engine {

	class Render2DUtils {



	public:
		static void DebugDumpFirstWords(const Engine::DeltaBitReader& rdr, uint32_t slot, uint32_t count = 500)
		{
			auto* w = rdr.GetTileWords(slot);
			if (!w) { printf("slot %u: null slice\n", slot); return; }
			printf("slot %u: first %u words:", slot, count);
			for (uint32_t i = 0; i < count; ++i) printf(" %08X", w[i]);
			printf("\n");
		}


		static inline void BarrierLayer(VkCommandBuffer cmd, VkImage img, uint32_t layer,
			VkImageLayout oldL, VkImageLayout newL,
			VkPipelineStageFlags srcStage, VkPipelineStageFlags dstStage,
			VkAccessFlags srcAccess, VkAccessFlags dstAccess)
		{
			VkImageMemoryBarrier b{};
			b.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
			b.srcAccessMask = srcAccess;
			b.dstAccessMask = dstAccess;
			b.oldLayout = oldL;
			b.newLayout = newL;
			b.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			b.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			b.image = img;
			b.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			b.subresourceRange.baseMipLevel = 0;
			b.subresourceRange.levelCount = 1;
			b.subresourceRange.baseArrayLayer = layer;
			b.subresourceRange.layerCount = 1;

			vkCmdPipelineBarrier(cmd, srcStage, dstStage, 0,
				0, nullptr, 0, nullptr, 1, &b);
		}

		static inline bool CircleIntersectsRect_HalfOpen(const glm::vec2& C, float r, const glm::vec2& minW, const glm::vec2& maxW_inclusive)
		{
			// make right/bottom edges exclusive via tiny shrink
			const float eps = 1e-6f;
			const glm::vec2 maxW = maxW_inclusive - glm::vec2(eps);

			// closest point on [min, max) box to the circle center
			glm::vec2 q = glm::clamp(C, minW, maxW);
			glm::vec2 d = C - q;
			return glm::dot(d, d) <= r * r;
		}


		static bool WorldToFxIdx(
			const glm::vec2& posW,
			const glm::vec2& fxGridTopLeftW,
			uint32_t& outFxIdx)
		{
			const glm::vec2 cellW((float)CHUNK_SIZE, (float)CHUNK_SIZE);

			const int col = (int)glm::floor((posW.x - fxGridTopLeftW.x) / cellW.x);
			const int row = (int)glm::floor((fxGridTopLeftW.y - posW.y) / cellW.y);

			const uint32_t fxIdx = (uint32_t)((CHUNK_GRID_WIDTH - 1 - row) * CHUNK_GRID_WIDTH + col);
			if (fxIdx >= CHUNK_GRID_SIZE)
				return false;

			outFxIdx = fxIdx;
			return true;
		}



		static void TransitionImageLayout(VkCommandBuffer commandBuffer, VkImage image,
			VkImageLayout oldLayout, VkImageLayout newLayout)
		{
			EE_PROFILE_FUNCTION();

			if (oldLayout == newLayout)
			{
				return;

			}

			VkImageMemoryBarrier barrier{};
			barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
			barrier.oldLayout = oldLayout;
			barrier.newLayout = newLayout;
			barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			barrier.image = image;
			barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			barrier.subresourceRange.baseMipLevel = 0;
			barrier.subresourceRange.levelCount = 1;
			barrier.subresourceRange.baseArrayLayer = 0;
			barrier.subresourceRange.layerCount = 1;

			//*********** src VkAccessFlags *****************
			// flag indicates which types of access to the image (or buffer) are 
			// required by the pipeline before the layout transition.
			// defines which operations or stages (such as reading or writing) need
			// to happen on the image before the layout change.
			// - VK_ACCESS_SHADER_READ_BIT: The image will be read by a shader.
			// - VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT: The image will be written to as a color attachment.
			// - VK_ACCESS_MEMORY_READ_BIT : General memory read access(for non - shader access).
			// - VK_ACCESS_MEMORY_WRITE_BIT : General memory write access.
			VkAccessFlags srcAccessMask = 0;

			//*********** dst VkAccessFlags *****************
			//  indicates the type of access after the layout transition has been completed.
			// defines the operations that will need access to the image in the new layout.
			VkAccessFlags dstAccessMask = 0;

			//************ sourceStage (VkPipelineStageFlags) *********
			//  specifies the pipeline stage during which the source access 
			// (specified by srcAccessMask) will occur before the layout transition.
			// ensures that the pipeline has finished all operations that 
			// need to occur before the transition
			VkPipelineStageFlags sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;

			// ******** destinationStage (VkPipelineStageFlags) ********
			// pecifies the pipeline stage after the layout transition, during which
			// the destination access (specified by dstAccessMask) will occur.
			// -VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT: Used when no specific stage is required.
			// -VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT: Used when you need to output to a color attachment.
			// -VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT : Used when you want to access the resource in a fragment shader.
			// -VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT : Used when you want to access the resource in a compute shader.
			VkPipelineStageFlags destinationStage = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;



			if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
			{
				dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
				destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
			}
			else if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL)
			{
				dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
				destinationStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
			}
			else if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL)
			{
				dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
				destinationStage = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
			}
			else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_GENERAL)
			{
				srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
				dstAccessMask = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT;

				sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
				destinationStage = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT | VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
			}
			else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
			{
				srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
				dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
				sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
				destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
			}
			else if (oldLayout == VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_PRESENT_SRC_KHR)
			{
				// Transition from COLOR_ATTACHMENT_OPTIMAL to PRESENT_SRC_KHR (for presentation)
				barrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
				barrier.dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
				sourceStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
				destinationStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
			}
			else if (oldLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL)
			{
				srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
				dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
				sourceStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
				destinationStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
			}
			else if (oldLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_GENERAL)
			{
				srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
				dstAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
				sourceStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
				destinationStage = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
			}
			else if (oldLayout == VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
			{
				srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
				dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
				sourceStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
				destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
			}
			else if (oldLayout == VK_IMAGE_LAYOUT_PRESENT_SRC_KHR && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
			{
				srcAccessMask = VK_ACCESS_MEMORY_READ_BIT;
				dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
				sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
				destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
			}
			else if (oldLayout == VK_IMAGE_LAYOUT_PRESENT_SRC_KHR && newLayout == VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL)
			{
				// Transition from PRESENT_SRC_KHR to COLOR_ATTACHMENT_OPTIMAL
				barrier.srcAccessMask = VK_ACCESS_MEMORY_READ_BIT; // Memory read during presentation
				barrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT; // Write access for color attachment
				sourceStage = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT; // After presentation
				destinationStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT; // Before rendering
			}
			else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL)
			{
				srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
				dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
				sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
				destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
			}
			else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
			{
				srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
				dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
				sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
				destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
			}
			else if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_GENERAL)
			{
				dstAccessMask = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT;
				destinationStage = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
			}
			else if (oldLayout == VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
				// Transition from COLOR_ATTACHMENT_OPTIMAL to SHADER_READ_ONLY_OPTIMAL
				barrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
				barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
				sourceStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
				destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
			}
			else if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_PRESENT_SRC_KHR) {
				// Transition from UNDEFINED to PRESENT_SRC_KHR (required for presenting)
				barrier.srcAccessMask = 0;
				barrier.dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
				sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
				destinationStage = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
			}
			else if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL)
			{
				// Transition from UNDEFINED to COLOR_ATTACHMENT_OPTIMAL (for rendering)
				barrier.srcAccessMask = 0;
				barrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
				sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
				destinationStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
			}
			else if (oldLayout == VK_IMAGE_LAYOUT_GENERAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
			{
				srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
				dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
				sourceStage = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
				destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
			}
			else
			{
				EE_CORE_ERROR("Unsupported layout transition: {} -> {}",
					VulkanUtils::LayoutToString(oldLayout),
					VulkanUtils::LayoutToString(newLayout));
			}

			barrier.srcAccessMask = srcAccessMask;
			barrier.dstAccessMask = dstAccessMask;


			// vkCmdPipelineBarrier command ensures that the proper synchronization occurs between
			// different stages of the Vulkan pipeline by specifying how and when the image will be used.



			//EE_CORE_INFO("Transitioning layout from {} to {}", VulkanUtils::LayoutToString(oldLayout), VulkanUtils::LayoutToString(newLayout));

			vkCmdPipelineBarrier(
				commandBuffer,
				sourceStage,
				destinationStage,
				0,
				0, nullptr,
				0, nullptr,
				1, &barrier
			);
		}

	};
}