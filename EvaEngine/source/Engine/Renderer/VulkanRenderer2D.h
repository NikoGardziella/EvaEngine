#pragma once

#include "Renderer2D.h"
#include "OrthographicCameraController.h"
#include "Engine/Renderer/EditorCamera.h"
#include "Engine/Platform/Vulkan/VulkanContext.h"
#include "Engine/Platform/Vulkan/VulkanDevice.h"
#include "Engine/Platform/Vulkan/VulkanBuffer.h"
#include "Engine/Platform/Vulkan/VulkanContext.h"
#include "Engine/Platform/Vulkan/VulkanTexture.h"
#include "Engine/Platform/Vulkan/VulkanContext.h"
#include "Engine/Platform/Vulkan/VulkanGraphicsPipeline.h"
#include <Engine/Platform/Vulkan/VulkanTrackedImage.h>

#include "vulkan/vulkan.h"
#include <glm/ext/matrix_float4x4.hpp>

namespace Engine {


	class VulkanRenderer2D
	{
	public:
		VulkanRenderer2D();
		~VulkanRenderer2D();
	
		void Init();
		void DrawFrame(uint32_t currentFrame);
		void BeginFrame(uint32_t currentFrame);
		void EndFrame(uint32_t currentFrame);

		static void StartBatch();
		static void NextBatch();
		static void Flush();

		// for rendering game in Editor
		VkDescriptorSet GetGameDescriptorSet(uint32_t index) const { return m_gameViewportDescriptorSets[index]; }

		static void DrawTextureQuad(const glm::mat4& transform, const std::shared_ptr<VulkanTexture>& texture, float tilingFactor, const glm::vec4& tintColor);
		static void DrawQuad(const glm::mat4& transform, const glm::vec4& color, int entityID = -1);
		static void BeginScene(const Camera& camera, const glm::mat4& transform);
		static void BeginScene(const EditorCamera& camera);
		static void BeginScene(glm::mat4 viewProjectionMatrix);
		static void EndScene();
		

		static Renderer2D::Statistics GetStats();
		static void ResetStats();

	private:

		void CreateImGuiTextureDescriptors();
		void RecordEditorDrawCommands(VkCommandBuffer commandBuffer, uint32_t imageIndex);
		void RecordGameDrawCommands(VkCommandBuffer commandBuffer, uint32_t imageIndex, uint32_t currentFrame);
		void RecordPresentDrawCommands(VkCommandBuffer commandBuffer, uint32_t imageIndex, uint32_t currentFrame);

		void AllocateCommandBuffers(VkDevice device, VkCommandPool commandPool);
		void CreateSyncObjects();

		// I dont use this at the moment, but they might come in handy later
		void TransitionImageLayout(VkCommandBuffer cmd, VkImage image, VkImageLayout oldLayout, VkImageLayout newLayout);

	private:

		// holds pipeline for game and present
		Ref<VulkanGraphicsPipeline> m_vulkanGraphicsPipelines;

		std::vector<VkCommandBuffer> m_commandBuffers;
		VulkanContext* m_vulkanContext;
		VkSwapchainKHR m_swapchain;
		VkExtent2D m_swapchainExtent;
		VkDevice m_device;

		std::vector<VkSemaphore> m_imageAvailableSemaphores;
		std::vector<VkSemaphore> m_renderFinishedSemaphores;
		std::vector<VkFence> m_inFlightFences;
		std::vector<VkFence> m_imagesInFlight;
		std::vector<VkDescriptorSet> m_gameViewportDescriptorSets;

		Ref<VulkanVertexBuffer> m_vertexBuffer;
		Ref<VulkanIndexBuffer> m_indexBuffer;

		Ref<OrthographicCamera> m_camera;
		uint32_t m_imageIndex;
		
	};



}
