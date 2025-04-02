#pragma once

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
		static void Init();
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
		static Ref<VulkanGraphicsPipeline> m_pipeline;
		static VkPipelineLayout m_pipelineLayout;
		static VkDescriptorPool m_descriptorPool;
		static VkCommandBuffer m_commandBuffer;
		static VkDescriptorSet m_descriptorSet;

		static VulkanVertexBuffer m_vertexBuffer;
		static VulkanIndexBuffer m_indexBuffer;
		static VulkanDevice m_device;
	};
}
