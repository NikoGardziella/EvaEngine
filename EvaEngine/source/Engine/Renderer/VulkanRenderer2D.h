#pragma once

#include "Renderer2D.h"
#include "VertexArray.h"
#include "Engine/Renderer/EditorCamera.h"
#include "Engine/Platform/Vulkan/VulkanContext.h"

#include "Engine/Platform/Vulkan/VulkanTexture.h"
#include "Engine/Platform/Vulkan/VulkanGraphicsPipeline.h"

#include "vulkan/vulkan.h"
#include <Engine/Events/Public/CollisionEvents.h>
#include <Engine/Scene/Entity.h>
#include <vector>

namespace Engine {

	enum eCollisionType {
		PROJECTILE = 0, // destroyed itself and what it hits
		PLAYER = 1, // only collision
		VEHICLE  = 2, // damage but, dont destroy itself
	};

	enum eComputeMode : uint32_t {
		DetectOnly = 0,
		Destroy = 1
	};

	struct VulkanRenderer2DProjectileData
	{
		static const uint32_t MaxProjectiles = MAX_PROJECTILES;

		Ref<VertexArray> QuadVertexArray;
		Ref<VulkanVertexBuffer> QuadVertexBuffer;
		Ref<VulkanIndexBuffer> QuadIndexBuffer;
		Ref<VulkanShader> QuadShader;
		Ref<VulkanTexture> WhiteTexture;

		uint32_t QuadIndexCount = 0;
		VulkanProjectileVertex* QuadVertexBufferBase = nullptr;
		VulkanProjectileVertex* QuadVertexBufferPtr = nullptr;

		std::unordered_map<VulkanTexture*, uint32_t> TextureToSlotMap;
		std::array<Ref<VulkanTexture>, MaxProjectiles> TextureSlots;
		uint32_t TextureSlotIndex = 0;

		glm::vec3 QuadVertexPositions[4];
	};


	struct VulkanRenderer2DData
	{
		static const uint32_t MaxQuads = 20000;
		static const uint32_t MaxVertices = MaxQuads * 4;
		static const uint32_t MaxIndices = MaxQuads * 6;
		static const uint32_t MaxTextureSlots = MAX_TEXTURES;
		static const uint32_t GridSize = CHUNK_GRID_WIDTH * CHUNK_GRID_WIDTH;

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

		std::unordered_map<VulkanTexture*, uint32_t> TextureToSlotMap;
		std::array<Ref<VulkanTexture>, MaxTextureSlots> TextureSlots;
		std::array<Ref<VulkanTexture>, GridSize> propertiesTextureSlots;
		std::array<Ref<VulkanTexture>, GridSize> GridTextureSlots;
		uint32_t TextureSlotIndex = 0;
		uint32_t GridSlotIndex = 0;
		//uint32_t HealthTextureSlotIndex = 0;

		glm::vec3 QuadVertexPositions[4];


		Renderer2D::Statistics Stats;

		struct CameraData
		{
			glm::mat4 ViewProjection;
		};
		CameraData CameraBuffer;
		//Ref<UniformBuffer> CameraUniformBuffer;
		uint32_t CurrentFrame = 0;

	};

	struct DestructibleSubmit {
		UUID		 entityID;
		glm::vec2    worldPos;   // center in world units
		glm::vec2    localPos;   // tile's local pos in entity space (for UID)
		glm::vec4    atlasUV;    // UNFLIPPED source UV in the atlas
		uint64_t     nameHash;   // hash of t.name to avoid storing strings
		float        zBias = 0.0f;
	};

	struct VulkanBindlessRenderer2DData
	{
		std::array<std::vector<DestructibleSubmit>, 3> submitQueues{};

	};

	struct CollisionData
	{

		uint32_t EntitySlotIndex = 0;

		std::array<CollisionEntitiesGPU, MAX_COLLISION_ENTITIES> CollisionEntities;
		std::array<CollisionPlayerEntitiesGPU, PLAYER_COUNT> playerEntities;
	};

	// Effect push constants for glow-only pass
	struct EffectPushConstants
	{
		glm::vec2  textureOrigin;
		float      pixelSize;
		uint32_t   textureIndex;

		uint32_t   defaultTimer;
		float      glowStrength;
		uint32_t   maxTimer;
		uint32_t   flags;           // bit0: glow on terrain, bit1: alpha-lift over empty

		glm::vec4  impactTint;      // rgb
		glm::vec4  destroyedTint;   // rgb
		glm::vec4  flashTint;       // rgb (reserved)

		glm::vec4  effectParams0;   // x=flashStrength, y=flickerAmount, z=alphaLiftEmpty, w=curveBoost
	};

	static_assert(sizeof(EffectPushConstants) == 96, "PC size must be 96 bytes");


	struct PushConstants
	{
		glm::vec2 TextureOrigin;   // 8 bytes
		float PixelSize;           // 4 bytes
		uint32_t textureIndex;     // 4 bytes

		uint32_t NumProjectiles;

		uint32_t ChunkSize; // in pixels
		uint32_t TileSize; // in pixels
		uint32_t mode; // 0 = Detect, 1 = Destroy, 2 = Tilemask
		glm::ivec2 MinTileCoords;
	
	};

	struct PerFrameGarbage
	{
		std::vector<std::shared_ptr<VulkanTexture>> OldTextures;
	};

	

	struct CollisionTexture
	{
		Ref<VulkanTexture> Textures[2]; // Ping-pong images
	};


	class VulkanRenderer2D
	{
	public:
		VulkanRenderer2D();
		~VulkanRenderer2D();

		void Init();
		void DrawFrame(uint32_t currentFrame);
		void BeginFrame(uint32_t currentFrame);
		void BeginPass(VkCommandBuffer cmd, uint32_t currentFrame);
		void EndFrame(uint32_t currentFrame);
		void SubmitFrame(VkCommandBuffer commandBuffer, uint32_t currentFrame);
		void BindBatchState(VkCommandBuffer cmd, uint32_t currentFrame);
		void SubmitFrame(uint32_t currentFrame);
		void CalculateCollisionFrame(uint32_t currentFrame);
		void ReadPlayerCollisionBuffer();
		void ReadBlockedTileMask(std::vector<uint32_t>& outDestroyedMask, uint32_t count);
		void DeviceWaitIdle();

		static void StartBatch();
		//static void FlushLines();
		static void NextBatch();
		static void Draw();

		// for rendering game in Editor
		VkDescriptorSet GetGameDescriptorSet(uint32_t index) const { return m_gameViewportDescriptorSets[index]; }
		
		static void CalculateBoxCollision(const glm::vec2& position, const glm::vec2& size, float rotation, uint64_t entityID, eCollisionType collisionType, uint32_t damage);
		static void CalculateCircleCollision(const glm::vec2& colliderPos, const float colliderRadius, uint64_t entityID,
			eCollisionType collisionType, uint32_t damage, const uint32_t destructionRadius, glm::vec2  projectileDirection,
			glm::vec2  TargetPositionAtFireTime, float  DistanceToTargetatFireTime, float  TargetPositionHeightZ1);
		static void CalculatePlayerCircleCollision(const glm::vec2& colliderPos, const float colliderRadius, uint64_t entityID, eCollisionType collisionType);
		static void DrawTextureQuadWithProperties(const glm::mat4& transform, const std::shared_ptr<VulkanTexture>& texture, const std::shared_ptr<VulkanTexture>& healthTexture);
		static void DrawTextureQuad(const glm::mat4& transform, const std::shared_ptr<VulkanTexture>& texture, float tilingFactor = 1, const glm::vec4& tintColor = glm::vec4(1));
		static void DrawProjectile(const glm::mat4& transform, const std::shared_ptr<VulkanTexture>& texture,const glm::vec4& tintColor);
		static void DrawQuad(const glm::mat4& transform, const glm::vec4& color, int entityID = -1);
		static void DrawLineRect(const glm::mat4& transform, const glm::vec4& color, int entityID = -1);
		static void DrawLine(const glm::vec3& p0, const glm::vec3& p1, const glm::vec4& color, int entityID = -1);
		static void DrawTile(const glm::vec2& worldPos, const glm::vec4& uv, const glm::vec4& color = glm::vec4(1));
		static void DrawTile(const glm::vec3& transform, const glm::vec4& uv, const glm::vec4& color);
		static void BeginScene(const Camera& camera, const glm::mat4& transform);
		static void BeginScene(const EditorCamera& camera);
		static void BeginScene(glm::mat4 viewProjectionMatrix);
		static void BeginScene();
		static void EndScene();

		static void VulkanRenderer2D::SubmitDestructibleTile(UUID entityID, const glm::vec2& worldPos,
			const glm::vec2& localPos, const glm::vec4& atlasUV, uint64_t nameHash, float zBias);


		static Renderer2D::Statistics GetStats();
		static EffectPushConstants& GetEffects() { return s_effectPushConstants;  }

		static void ResetStats();

	private:

		void CreateImGuiTextureDescriptors();
		void RecordEditorDrawCommands(VkCommandBuffer commandBuffer, uint32_t imageIndex, uint32_t currentFrame);
		void RecordGameDrawCommands(VkCommandBuffer commandBuffer, uint32_t imageIndex, uint32_t currentFrame);
		void RecordProjectileDrawCommands(VkCommandBuffer cmd, uint32_t frameIndex, uint32_t currentFrame);
		void RecordPresentDrawCommands(VkCommandBuffer commandBuffer, uint32_t imageIndex, uint32_t currentFrame);
		void RecordComputeCommanedBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex, uint32_t currentFrame);
		void RecordPlayerCommandBuffer(VkCommandBuffer cmd, uint32_t imageIndex, uint32_t currentFrame);
		void RecordEffectComputeCommandBuffer(VkCommandBuffer cmdBuf, uint32_t currentFrame);
		void RecordLineCommanedBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex, uint32_t currentFrame);
		void ConsumeDestructibleQueue(VkCommandBuffer uploadCB, uint32_t frameIndex);
		void AllocateCommandBuffers(VkDevice device, VkCommandPool commandPool);
		void CreateSyncObjects();


		void TransitionImageLayout(VkCommandBuffer cmd, VkImage image, VkImageLayout oldLayout, VkImageLayout newLayout);

	private:

		// holds pipeline for game and present
		Ref<VulkanGraphicsPipeline> m_vulkanGraphicsPipelines;

		std::vector<VkCommandBuffer> m_commandBuffers;
		std::vector<VkCommandBuffer> m_endFrameCommandBuffers;
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

		uint32_t inputIndex = 0;
		uint32_t outputIndex = 0;
		static VulkanRenderer2DData s_VulkanData;
		static VulkanBindlessRenderer2DData s_VulkanBindlessData;
		static VulkanRenderer2DProjectileData s_VulkanProjectileData;
		static CollisionData s_CollisionData;
		static EffectPushConstants s_effectPushConstants;
		//static const uint32_t MaxTextures = 10;
		//std::array<CollisionTexture, MaxTextures> s_CollisionTextures;
		//CollisionTexture s_CollisionTextures;
		Ref<VulkanTexture> m_dummyTexture;


	};



}
