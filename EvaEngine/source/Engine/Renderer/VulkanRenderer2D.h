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
#include "Engine/Renderer/EditorCamera.h"
#include "Renderer2D.h"

namespace Engine {




	class VulkanRenderer2D
	{
	public:
		VulkanRenderer2D();
		~VulkanRenderer2D();

		void Init();
		void DrawFrame(uint32_t currentFrame);
		void RecordEditorDrawCommands(VkCommandBuffer commandBuffer, uint32_t imageIndex);
		void RecordGameDrawCommands(VkCommandBuffer commandBuffer, uint32_t imageIndex);
		void TransitionImageLayout(VkCommandBuffer commandBuffer, VkImage image, VkImageLayout newLayout, VkImageSubresourceRange subresourceRange);
		//inline VulkanGraphicsPipeline* GetGraphicsPipeline() { return m_vulkanGraphicsPipeline.get(); }
		VkDescriptorSet GetGameDescriptorSet(uint32_t index) const { return m_gameViewportDescriptorSets[index]; }
		void TransitionImageForShaderRead(VkCommandBuffer cmd, uint32_t imageIndex, VkImage image, VulkanContext* vulkanContext);

		static void DrawTextureQuad(const glm::mat4& transform, const std::shared_ptr<VulkanTexture>& texture, float tilingFactor, const glm::vec4& tintColor);
		static void DrawQuad(const glm::mat4& transform, const glm::vec4& color, int entityID = -1);
		static void BeginScene(const Camera& camera, const glm::mat4& transform);
		static void BeginScene(const EditorCamera& camera);
		static void BeginScene(glm::mat4 viewProjectionMatrix);
		static void EndScene();

		void CreateImGuiTextureDescriptors();

		static Renderer2D::Statistics GetStats();

		static void ResetStats();
		

	private:

		void AllocateCommandBuffers(VkDevice device, VkCommandPool commandPool);
		void RecordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex);
		void TransitionImageLayout(VkCommandBuffer cmd, VkImage image, VkImageLayout oldLayout, VkImageLayout newLayout);
		void TransitionImage(VkCommandBuffer cmd, VkImage image, VkImageLayout oldLayout, VkImageLayout newLayout, VkImageAspectFlags aspect);
		void CreateSyncObjects();

		void PrepareImageForRenderPass(VkCommandBuffer commandBuffer, uint32_t imageIndex);

		void TransitionToPresent(VkCommandBuffer commandBuffer, uint32_t imageIndex);


		//void UpdateDescriptorSet(VkDescriptorSet descriptorSet, const VulkanBuffer& uniformBuffer, VkImageView textureImageView, VkSampler textureSampler);
	private:

		Ref<VulkanGraphicsPipeline> m_vulkanGraphicsPipeline;

		std::vector<VkCommandBuffer> m_commandBuffers;
		std::vector<VkCommandBuffer> m_imGuiCommandBuffers;
		VulkanContext* m_vulkanContext;
		VkSwapchainKHR m_swapchain;
		VkExtent2D m_swapchainExtent;
		VkDevice m_device;

		std::vector<VkSemaphore> m_imageAvailableSemaphores;
		std::vector<VkSemaphore> m_renderFinishedSemaphores;
		std::vector<VkFence> m_inFlightFences;
		std::vector<VkDescriptorSet> m_gameViewportDescriptorSets;

		std::vector<VkImageLayout> m_imageLayouts;
		std::vector<VkImageLayout> m_gameColorLayouts;

		Ref<VulkanVertexBuffer> m_vertexBuffer;
		Ref<VulkanIndexBuffer> m_indexBuffer;

		Ref<OrthographicCamera> m_camera;


		//********** Experiental **********
		struct SceneData
		{
			glm::mat4 ViewProjectionMatrix;
		};
		//static SceneData* m_sceneData;


		static std::vector<VulkanQuadVertex> s_QuadVertices;
		static std::vector<uint32_t> s_QuadIndices;

		static glm::vec4 QuadVertexPositions[4];
		
		static const uint32_t MaxTextureSlots = 32;

		
		//static inline RendererStats s_Stats;

		// Define quad vertices for a textured quad
		


	};


}
