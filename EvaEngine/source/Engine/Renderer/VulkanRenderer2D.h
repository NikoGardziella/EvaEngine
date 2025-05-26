#pragma once

#include "Renderer2D.h"
#include "VertexArray.h"
#include "Engine/Renderer/EditorCamera.h"
#include "Engine/Platform/Vulkan/VulkanContext.h"

#include "Engine/Platform/Vulkan/VulkanTexture.h"
#include "Engine/Platform/Vulkan/VulkanGraphicsPipeline.h"

#include "vulkan/vulkan.h"

namespace Engine {

	struct VulkanRenderer2DData
	{
		static const uint32_t MaxQuads = 20000;
		static const uint32_t MaxVertices = MaxQuads * 4;
		static const uint32_t MaxIndices = MaxQuads * 6;
		static const uint32_t MaxTextureSlots = 32; // TODO: RenderCaps

		static const uint32_t MaxLines = 10000;
		static const uint32_t MaxLineVertices = MaxLines * 2;

		Ref<VulkanBuffer> LineStagingBuffer;
		VulkanLineVertex* LineVertexBufferBase = nullptr;
		VulkanLineVertex* LineVertexBufferPtr = nullptr;
		uint32_t LineVertexCount = 0;
		Ref<VulkanBuffer> LineVertexBuffer;
		Ref<VertexArray> LineVertexArray;

		Ref<VertexArray> QuadVertexArray;
		Ref<VulkanVertexBuffer> QuadVertexBuffer;
		Ref<VulkanIndexBuffer> QuadIndexBuffer;
		Ref<VulkanShader> QuadShader;
		Ref<VulkanTexture> WhiteTexture;

		uint32_t QuadIndexCount = 0;
		VulkanQuadVertex* QuadVertexBufferBase = nullptr;
		VulkanQuadVertex* QuadVertexBufferPtr = nullptr;

		std::array<Ref<VulkanTexture>, MaxTextureSlots> TextureSlots;
		std::array<Ref<VulkanPixelTexture>, MaxTextureSlots> PixelTextureSlots;
		uint32_t TextureSlotIndex = 1; // 0 = white texture

		glm::vec3 QuadVertexPositions[4];


		Renderer2D::Statistics Stats;

		struct CameraData
		{
			glm::mat4 ViewProjection;
		};
		CameraData CameraBuffer;
		//Ref<UniformBuffer> CameraUniformBuffer;
	};


	struct CollisionData
	{
		static const uint32_t MaxTextures = 32;
		glm::vec2 playerPos;
		float radius;
		std::array<glm::vec2, MaxTextures> Textures;
		float PixelSize;
		uint32_t TextureSlotIndex = 0;
	};

	class VulkanRenderer2D
	{
	public:
		VulkanRenderer2D();
		~VulkanRenderer2D();
	
		void Init();
		void DrawFrame(uint32_t currentFrame);
		void BeginFrame(uint32_t currentFrame);
		void EndFrame(uint32_t currentFrame);
		void DeviceWaitIdle();

		static void StartBatch();
		static void FlushLines();
		static void NextBatch();
		static void Draw();

		// for rendering game in Editor
		VkDescriptorSet GetGameDescriptorSet(uint32_t index) const { return m_gameViewportDescriptorSets[index]; }

		static void CalculateCollision(glm::vec2& textureOrigin, const float pixelSize, const glm::vec2& playerPos, const float radius);
		static void DrawTextureQuad(const glm::mat4& transform, const std::shared_ptr<VulkanTexture>& texture, float tilingFactor, const glm::vec4& tintColor);
		static void DrawQuad(const glm::mat4& transform, const glm::vec4& color, int entityID = -1);
		static void DrawLineRect(const glm::mat4& transform, const glm::vec4& color, int entityID);
		static void DrawLine(const glm::vec3& p0, const glm::vec3& p1, const glm::vec4& color, int entityID);
		static void BeginScene(const Camera& camera, const glm::mat4& transform);
		static void BeginScene(const EditorCamera& camera);
		static void BeginScene(glm::mat4 viewProjectionMatrix);
		static void BeginScene();
		static void EndScene();
		

		static Renderer2D::Statistics GetStats();
		static void ResetStats();

	private:

		void CreateImGuiTextureDescriptors();
		void RecordEditorDrawCommands(VkCommandBuffer commandBuffer, uint32_t imageIndex, uint32_t currentFrame);
		void RecordGameDrawCommands(VkCommandBuffer commandBuffer, uint32_t imageIndex, uint32_t currentFrame);
		void RecordPresentDrawCommands(VkCommandBuffer commandBuffer, uint32_t imageIndex, uint32_t currentFrame);
		void RecordComputeCommanedBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex, uint32_t currentFrame);
		void RecordLineCommanedBuffer(VkCommandBuffer commandBuffer);

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

		VkFence m_computeFence;


		std::vector<VkDescriptorSet> m_gameViewportDescriptorSets;

	
		Ref<OrthographicCamera> m_camera;
		uint32_t m_imageIndex;
		uint32_t m_firstIndex = 0;
		uint32_t m_vertexOffset = 0;
		
		static VulkanRenderer2DData s_VulkanData;
		static CollisionData s_CollisionData;

	};



}
