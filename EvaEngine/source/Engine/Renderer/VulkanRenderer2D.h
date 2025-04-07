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
		~VulkanRenderer2D();

		void Init();
		void DrawFrame();
	private:

		void AllocateCommandBuffers(VkDevice device, VkCommandPool commandPool);
		void RecordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex);
		void CreateSyncObjects();

		//void UpdateDescriptorSet(VkDescriptorSet descriptorSet, const VulkanBuffer& uniformBuffer, VkImageView textureImageView, VkSampler textureSampler);
		void UpdateDescriptorSet();
	private:

		Ref<VulkanGraphicsPipeline> m_vulkanGraphicsPipeline;

		std::vector<VkCommandBuffer> m_commandBuffers;
		VulkanContext* m_vulkanContext;
		VkSwapchainKHR m_swapchain;
		VkExtent2D m_swapchainExtent;
		VkDevice m_device;

		std::vector<VkSemaphore> m_imageAvailableSemaphores;
		std::vector<VkSemaphore> m_renderFinishedSemaphores;
		std::vector<VkFence> m_inFlightFences;

		uint32_t m_currentFrame = 0;


		Ref<VertexBuffer> m_vertexBuffer;
		Ref<IndexBuffer> m_indexBuffer;


		struct QuadVertex
		{
			glm::vec3 Position;  // Vertex position (x, y, z)
			glm::vec4 Color;     // Vertex color (r, g, b, a)
			glm::vec2 TexCoord;  // Texture coordinates (u, v)
			float TexIndex;      // Texture index for binding
			float TilingFactor;  // Tiling factor for the texture
		};

		// Define quad vertices for a textured quad
		const std::vector<QuadVertex> quadVertices =
		{
			// Position              Color                   TexCoord    TexIndex  TilingFactor
			// Bottom Left
			{{-0.5f, -0.5f, 0.0f}, {1.0f, 0.0f, 0.0f, 1.0f}, {0.0f, 0.0f}, 0.0f, 1.0f},
			// Bottom Right
			{{ 0.5f, -0.5f, 0.0f}, {0.0f, 1.0f, 0.0f, 1.0f}, {1.0f, 0.0f}, 0.0f, 1.0f},
			// Top Right
			{{ 0.5f,  0.5f, 0.0f}, {0.0f, 0.0f, 1.0f, 1.0f}, {1.0f, 1.0f}, 0.0f, 1.0f},
			// Top Left
			{{-0.5f,  0.5f, 0.0f}, {1.0f, 1.0f, 1.0f, 1.0f}, {0.0f, 1.0f}, 0.0f, 1.0f}
		};


		std::vector<uint32_t> quadIndices =
		{
			0, 1, 2, 2, 3, 0 // Two triangles forming a quad
		};


	};


}
