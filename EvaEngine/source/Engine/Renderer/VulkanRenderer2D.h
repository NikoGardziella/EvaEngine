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
#include "OrthographicCameraController.h"


namespace Engine {




	class VulkanRenderer2D
	{
	public:
		VulkanRenderer2D();
		~VulkanRenderer2D();

		void Init();
		void DrawFrame(uint32_t currentFrame);


		static void DrawQuad(const glm::mat4& transform, const std::shared_ptr<VulkanTexture>& texture, float tilingFactor, const glm::vec4& tintColor);
		static void BeginScene(glm::mat4 viewProjectionMatrix);
		static void EndScene();

	private:

		void AllocateCommandBuffers(VkDevice device, VkCommandPool commandPool);
		void RecordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex);
		void CreateSyncObjects();



		//void UpdateDescriptorSet(VkDescriptorSet descriptorSet, const VulkanBuffer& uniformBuffer, VkImageView textureImageView, VkSampler textureSampler);
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

		//uint32_t currentFrame = 0;


		Ref<VulkanVertexBuffer> m_vertexBuffer;
		Ref<VulkanIndexBuffer> m_indexBuffer;

		Ref<OrthographicCamera> m_camera;



		//********** Experiental **********
		struct SceneData
		{
			glm::mat4 ViewProjectionMatrix;
		};
		static SceneData* m_sceneData;


		static std::vector<VulkanQuadVertex> s_QuadVertices;
		static std::vector<uint32_t> s_QuadIndices;

		static glm::vec4 QuadVertexPositions[4];
		
		static const uint32_t MaxTextureSlots = 32;

		static inline uint32_t s_TextureSlotIndex = 1;
		static inline std::shared_ptr<VulkanTexture> s_TextureSlots[MaxTextureSlots];
		static inline uint32_t s_QuadIndexCount = 0;
		static inline VulkanQuadVertex* s_QuadVertexBufferPtr = nullptr;
		static inline VkPipeline s_Pipeline;
		static inline VkPipelineLayout s_PipelineLayout;
		static inline VkDescriptorSet s_DescriptorSet;
		static inline VkBuffer s_QuadVertexBuffer;
		static inline VkBuffer s_QuadIndexBuffer;
		static inline size_t s_QuadVertexBufferOffset = 0;
		//static inline RendererStats s_Stats;

		// Define quad vertices for a textured quad
		const std::vector<VulkanQuadVertex> quadVertices =
		{
			// Position              Color                   TexCoord    TexIndex  TilingFactor
			// Bottom Left
			{{-0.5f, -0.5f, 0.0f}, {1.0f, 0.0f, 0.0f, 1.0f}, {0.0f, 0.0f}, 0.0f, 1.0f},
			// Bottom Right	0
			{{ 0.5f, -0.5f, 0.0f}, {0.0f, 1.0f, 0.0f, 1.0f}, {1.0f, 0.0f}, 0.0f, 1.0f},
			// Top Right	0
			{{ 0.5f,  0.5f, 0.0f}, {0.0f, 0.0f, 1.0f, 1.0f}, {1.0f, 1.0f}, 0.0f, 1.0f},
			// Top Left		0
			{{-0.5f,  0.5f, 0.0f}, {1.0f, 1.0f, 1.0f, 1.0f}, {0.0f, 1.0f}, 0.0f, 1.0f}
		};


		std::vector<uint32_t> quadIndices =
		{
			0, 1, 2, 2, 3, 0 // Two triangles forming a quad
		};


	};


}
