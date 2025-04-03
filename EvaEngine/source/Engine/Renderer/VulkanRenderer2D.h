#pragma once

#include "vulkan/vulkan.h"
#include "Engine/Platform/Vulkan/VulkanContext.h"
#include "Engine/Platform/Vulkan/VulkanDevice.h"
#include "Engine/Platform/Vulkan/VulkanBuffer.h"
#include "Engine/Platform/Vulkan/VulkanContext.h"
#include "Engine/Platform/Vulkan/VulkanTexture.h"
#include "Engine/Platform/Vulkan/VulkanContext.h"
#include "Engine/Platform/Vulkan/VulkanGraphicsPipeline.h"
#include <glm/ext/matrix_float4x4.hpp>


namespace Engine {

	class VulkanRenderer2D
	{
	public:
		VulkanRenderer2D();
		~VulkanRenderer2D() = default;

		void Init();
		void AllocateCommandBuffers(VkDevice device, VkCommandPool commandPool);
		void RecordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex);
		/*
		static void Shutdown();

		static void BeginScene(const glm::mat4& viewProjectionMatrix);
		static void EndScene();

		static void DrawQuad(const glm::vec3& position, const glm::vec2& size, const glm::vec4& color);
		static void DrawQuad(const glm::vec3& position, const glm::vec2& size, const Ref<VulkanTexture>& texture, const glm::vec4& tintColor = glm::vec4(1.0f));

		static void Flush(); // Submit command buffer

	private:
		static void CreatePipeline();
		static void CreateBuffers();
		static void CreateDescriptors();

		*/
	private:

		Ref<VulkanGraphicsPipeline> m_vulkanGraphicsPipeline;

		std::vector<VkCommandBuffer> m_commandBuffers;
		VulkanContext* m_vulkanContext;
		VulkanSwapchain* m_swapchain;
		VkExtent2D* m_swapchainExtent;
	};
}
