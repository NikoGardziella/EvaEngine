#pragma once

#include <Engine/Core/Config.h>
#include <Engine/Core/Core.h>
#include <Engine/Platform/Vulkan/VulkanShader.h>
#include <Engine/Scene/SceneCamera.h>
#include <vulkan/vulkan_core.h>


#include "Engine/Renderer/EditorCamera.h"
#include "Engine/Platform/Vulkan/VulkanContext.h"

#include "Engine/Platform/Vulkan/VulkanTexture.h"
#include "Engine/Platform/Vulkan/VulkanGraphicsPipeline.h"

#include "vulkan/vulkan.h"
#include <vector>
#include <Engine/Platform/Vulkan/VulkanBindlessDescriptorSet.h>

#include <Engine/Platform/Vulkan/VulkanFogOfWarPipelines.h>
#include "Engine/Renderer/Utils/PlayerData.h"
#include <Engine/Map/TextureStreaming/TextureStreamingSystem.h>
#include <Engine/Platform/Vulkan/VulkanBuffer.h>
#include <unordered_map>
#include <array>

#include <glm/glm.hpp>
#include <Engine/Renderer/Renderer2D.h>
#include <Engine/Scene/Components/Render/TileComponent.h>
#include <Engine/Renderer/VertexArray.h>

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



	struct TileToDestroy
	{
		uint32_t slot;
		uint32_t newSlot;
		std::vector<uint32_t> words;
		int cutY;
	};

	struct VulkanRenderer2DTileDestructionData
	{
		std::vector<TileToDestroy> TilesDestroyQueu;
		uint32_t TileToDestroyIndex = 0; 

	};

	struct VulkanRenderer2DData
	{
		static const uint32_t MaxQuads = 4000;
		static const uint32_t MaxVertices = MaxQuads * 4;
		static const uint32_t MaxIndices = MaxQuads * 6;
		static const uint32_t MaxTextureSlots = MAX_TEXTURES;
		static const uint32_t GridSize = CHUNK_GRID_WIDTH * CHUNK_GRID_WIDTH;

		static const uint32_t MaxLines = 10000;
		static const uint32_t MaxLineVertices = MaxLines * 2;

		//Ref<VulkanBuffer> LineStagingBuffer;
		VulkanLineVertex* LineVertexBufferBase = nullptr;
		VulkanLineVertex* LineVertexBufferPtr = nullptr;
		uint32_t LineVertexCount = 0;
		Ref<VulkanBuffer> LineVertexBuffer;


		VulkanLineVertex* LineVertexBufferBaseUnderlay = nullptr;
		VulkanLineVertex* LineVertexBufferPtrUnderlay = nullptr;
		uint32_t LineVertexCountUnderlay = 0;
		Ref<VulkanBuffer> LineVertexBufferUnderlay;

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
		std::array<Ref<VulkanTexture>, GridSize> VisualEffectsTextureSlots;
		std::array<Ref<VulkanTexture>, GridSize> propertiesTextureSlots;
		std::array<Ref<VulkanTexture>, GridSize> GridTextureSlots;

		std::unordered_map<uint64_t, uint32_t> TextureSlotLUT;

		uint32_t TextureSlotIndex = 0;
		uint32_t VisualTextureSlotIndex = 0;
		uint32_t GridSlotIndex = 0;
		//uint32_t HealthTextureSlotIndex = 0;

		glm::vec3 QuadVertexPositions[4];


		Renderer2D::Statistics Stats;

		struct CameraData
		{
			glm::mat4 ViewProjection = glm::mat4(1.0f);

			glm::vec2 viewportPx = glm::vec2(1.0f);
		};
		CameraData CameraBuffer;
		//Ref<UniformBuffer> CameraUniformBuffer;
		uint32_t CurrentFrame = 0;

		struct FogData
		{
			VkBuffer buffer = VK_NULL_HANDLE;
			VkDeviceMemory memory = VK_NULL_HANDLE;
			void* mapped = nullptr;

			uint32_t capacityVertices = 0;
			uint32_t cursorVertices = 0;
			VulkanFogOfWarPipelines::FogSubmitResult submitResult;
		};
		FogData Fog;

	};
	struct SlotContentRect { glm::ivec2 minPx; glm::ivec2 sizePx; };

	struct DestructibleSubmit {
		glm::vec2		worldPos;   // center in world units
		glm::vec2		localPos;   // tile's local pos in entity space (for UID)
		glm::vec4		atlasUV;    // UNFLIPPED source UV in the atlas
		uint64_t		nameHash;   // hash of t.name to avoid storing strings // UID
		float			zBias = 0.0f;
		eTileDirection	tileDirection; // N,S,W,E directions for shadows.
		glm::ivec2		outOpaqueMin = glm::ivec2(TILE_PIXEL_WIDTH, TILE_PIXEL_HEIGHT);
		glm::ivec2		outOpaqueMax = glm::ivec2(0);
		uint32_t		flags;
	};

	struct SpriteSubmit {
		glm::vec2	center;     // world center (match your shader convention)
		float		zKey;       // painter’s sort key you computed
		float		rotation;
		uint32_t	slot;       // spritesheet bindless slot (binding=3)
		glm::uvec2	uvMin16;   // frame UVs (quantized 0..65535)
		glm::uvec2	uvMax16;
		glm::vec2	sizeWorld;  // frame size in world units
	};


	struct VulkanBindlessRenderer2DData
	{
		std::array<std::vector<DestructibleSubmit>, MAX_FRAMES_IN_FLIGHT> submitQueues{};
		std::array<std::vector<SpriteSubmit>, MAX_FRAMES_IN_FLIGHT> spriteSubmitQueues{};
		std::vector<glm::vec2> m_slotOriginWorld;


	};



	struct CPUExplosion
	{

		glm::vec2	HitWorldPos;
		float		radiWorld;
		uint32_t	damage;
	};

	struct CPUExplosionData
	{

		std::vector<CPUExplosion> CPUExplosions;
	};

	

	struct CollisionData
	{

		uint32_t EntitySlotIndex = 0;

		std::array<CollisionEntitiesGPU, MAX_COLLISION_ENTITIES> CollisionEntities;
		std::array<CollisionPlayerEntitiesGPU, PLAYER_COUNT> playerEntities;
		std::vector<uint64_t> AffectedSlots;
	};

	// Effect push constants for glow-only pass
	struct EffectPushConstants {
		glm::vec2  textureOrigin;   // 0..7
		float      pixelSize;       // 8..11
		uint32_t   textureIndex;    // 12..15

		uint32_t   defaultTimer;    // 16..19
		float      glowStrength;    // 20..23
		uint32_t   maxTimer;        // 24..27
		uint32_t   flags;           // 28..31


		glm::vec4  flashTint;       // 64..79
		glm::vec4  effectParams0;   // 80..95

		float      hitRadiusWS;     // 96..99
		uint32_t   hitDamage;       // 100..103
		uint32_t      fxIdx;           // 104..107

		uint32_t   mode;
		glm::vec2  fxTextureOrigin; // 112..119
		uint32_t   cutY;
		uint32_t   newSlot;
		glm::vec2  impactCenterWorld;
	};
	//static_assert(offsetof(EffectPushConstants, fxTextureOrigin) == 112, "pad needed");

	// C++ (matches GLSL layout+offsets; total = 96 bytes)
	struct ComputePC {
		glm::vec2  TextureOriginWorld; // TOP-LEFT of the tile in world units
		float      PixelSizeWorld;     // world units per pixel (same X/Y in shader)
		uint32_t   TextureIndex;       // = slot
		uint32_t   NumProjectiles;
		uint32_t   TileSizePixels;     // tile width in texels (e.g. 128)	
	};


	struct ClearMaskPC {
		uint32_t Width;       // TILE_PIXEL_WIDTH
		uint32_t Height;      // TILE_PIXEL_HEIGHT
		uint32_t LsbFirst;    // 0 or 1
		uint32_t ClearFlags;  // 0 or your tag
		uint32_t CutY;        // (uint32_t)cutY or 0xFFFFFFFF
	};
	static_assert(sizeof(ClearMaskPC) % 4 == 0, "Push constants must be 4-byte aligned");

	struct AffectedTile
	{
		uint32_t	slot = 0;
		uint32_t    totalDamage = 0; // sum of damage of all hits affecting this tile
		float		maxRadius = 0.0f; // max radius among hits affecting this tile
		glm::vec2	impactCenterWorld;
		uint32_t    hitIndex;
	};

	

	class VulkanUIRenderer;
	class VulkanRenderer2D
	{
	

	public:

		struct GroundPC {
			glm::mat4 lightSpaceMatrix;
			glm::vec4 lightDirection;
		};



	public:
		VulkanRenderer2D();
		~VulkanRenderer2D();

		void Init(Ref<VulkanShadowMap> shadowMap);
		void DrawFrame(uint32_t currentFrame, VkCommandBuffer cmd);
		void DrawTiles(uint32_t currentFrame, VkCommandBuffer cmd, Ref<VulkanShadowMap> shadowMap);
		void DrawTilesShadowPass(VkCommandBuffer cmd, uint32_t frameIndex, Ref<VulkanShadowMap> shadowMap);
		void ReadAndResetCollisionBuffer(uint32_t currentFrame);
		void BeginFrame(uint32_t currentFrame);
		void EndFrame(uint32_t currentFrame, VkCommandBuffer cmd);

		void CalculateCollisionFrame(uint32_t currentFrame, VkCommandBuffer cmd);
		void ReadBlockedTileMask(std::vector<uint32_t>& outDestroyedMask, uint32_t count);
		bool ReadDirtyOut();
		bool ClearAliveBitsHost();
		void DeviceWaitIdle();

		static void StartBatch();
		//static void FlushLines();
		static void NextBatch();
		static void Draw();

		// for rendering game in Editor
		VkDescriptorSet GetGameDescriptorSet(uint32_t index) const { return m_gameViewportDescriptorSets[index]; }
		VkDescriptorSet GetCurrentGameDescriptorSet() const 
		{
			if (m_imageIndex >= m_gameViewportDescriptorSets.size())
			{
				return VK_NULL_HANDLE;
			}

			return m_gameViewportDescriptorSets[m_imageIndex];
		}
		Ref<VulkanGraphicsPipeline> GetGraphicsPipelines() { return m_vulkanGraphicsPipelines; };

		static void CalculateBoxCollision(const glm::vec2& position, const glm::vec2& size, float rotation, uint64_t entityID, eCollisionType collisionType, uint32_t damage);
		static void CalculateBoxCollision(const glm::vec2& position, const glm::vec2& size, float rotation, uint64_t entityID, eCollisionType collisionType, uint32_t damage, const std::vector<uint64_t>& affectedSlots);
		static void CalculateCircleCollision(const glm::vec2& colliderPos, const float colliderRadius, uint64_t entityID,
			eCollisionType collisionType, uint32_t damage, const float destructionRadius, glm::vec2  projectileDirection,
			glm::vec2  TargetPositionAtFireTime, float  DistanceToTargetatFireTime, float  TargetPositionHeightZ1);
		static void CalculateCircleCollision(const glm::vec2& colliderPos, float colliderRadius, uint64_t entityID, eCollisionType collisionType, uint32_t damage, float destructionRadius, glm::vec2 projectileDirection, glm::vec2 targetPositionAtFireTime, float distanceToTargetAtFireTime, float targetPositionHeightZ1, const std::vector<uint64_t>& affectedSlots);
		static void CalculatePlayerCircleCollision(const glm::vec2& colliderPos, const float colliderRadius, uint64_t entityID, eCollisionType collisionType);
		static void DrawTextureQuadWithProperties(const glm::mat4& transform, const std::shared_ptr<VulkanTexture>& texture, const std::shared_ptr<VulkanTexture>& healthTexture);
		static void DrawTextureQuad(glm::mat4& transform, const std::shared_ptr<VulkanTexture>& texture, float tilingFactor = 1, const glm::vec4& tintColor = glm::vec4(1));
		static void DrawQuadRaw(const glm::vec3& p0, const glm::vec3& p1, const glm::vec3& p2, const glm::vec3& p3, const glm::vec2& uv0, const glm::vec2& uv1, const glm::vec2& uv2, const glm::vec2& uv3, const std::shared_ptr<VulkanTexture>& texture, float tilingFactor, const glm::vec4& tintColor);
		static uint32_t AcquireTextureSlot(const std::shared_ptr<VulkanTexture>& texture);
		static void DrawVisualEffectTexture(const glm::mat4& transform, const std::shared_ptr<VulkanTexture>& texture);
		static void DrawQuad(const glm::mat4& transform, const glm::vec4& color, int entityID = -1);
		static void DrawLineRect(const glm::mat4& transform, const glm::vec4& color, int entityID = -1);
		static void DrawLine(const glm::vec3& p0, const glm::vec3& p1, const glm::vec4& color, int entityID = -1);
		static void DrawLineUnderlay(const glm::vec3& p0, const glm::vec3& p1, const glm::vec4& color, int entityID = -1);
		static void VulkanRenderer2D::DrawTile(const glm::vec2& worldPos, const glm::vec4& uv, const glm::vec4& color, float zOverride = 0.0f);
		static void DrawTile(const glm::vec3& transform, const glm::vec4& uv, const glm::vec4& color);
		static void RemoveTilePixels(const uint32_t slot, const uint32_t newSlot, const std::vector<uint32_t>& words, const int cutY);
		static void BeginScene(const SceneCamera& camera, const glm::mat4& transform);
		static void BeginScene(const EditorCamera& camera);
		static void BeginScene(glm::mat4 viewProjectionMatrix);
		static void BeginScene();
		static void EndScene();

		static void SubmitDestructibleTile(const glm::vec2& worldPos, const glm::vec2& localPos, const glm::vec4& atlasUV, 
			uint64_t nameHash, float zBias, eTileDirection tileDirection, const glm::ivec2 outOpaqueMin, const glm::ivec2 outOpaqueMa, uint32_t flags);

		static void SubmitAnimationSpriteInstance(glm::vec2 worldCenter, float zKey, uint32_t spriteSlot, glm::uvec2 uvMin16, glm::uvec2 uvMax16, glm::vec2 sizeWorld, float rotation);

		void RenderVisibilityMap(VkCommandBuffer cmd, uint32_t frameIndex);

		static void SubmitFogGeometry(const std::vector<VulkanFogOfWarPipelines::FogVertex>& fanTris, const std::vector<VulkanFogOfWarPipelines::FogVertex>& quadTris);
		static void ClearFogSubmission();


		static void SubmitCPUExplosion(glm::vec2 HitWorldPos, float radiWorld, uint32_t damage);

		static void SubmitPlayerData(PlayerData playerStateData);

		static Renderer2D::Statistics GetStats();
		static EffectPushConstants& GetEffects() { return s_effectPushConstants;  }
		static Ref<VulkanBindlessDescriptorSetRenderer>& GetBindlessDescriptorSetRenderer() { return s_bindlessDescitproRenderer;  }

		static void ResetStats();

		void DrawFogOverlay(VkCommandBuffer cmd);


		void RecordUnderlayLineCommanedBuffer(VkCommandBuffer commandBuffer, uint32_t currentFrame);
	private:

		void CreateImGuiTextureDescriptors();
		void RecordEditorDrawCommands(VkCommandBuffer commandBuffer, uint32_t imageIndex, uint32_t currentFrame);
		void RecordGameDrawCommands(VkCommandBuffer commandBuffer, uint32_t currentFrame);
		void RecordPresentDrawCommands(VkCommandBuffer commandBuffer, uint32_t imageIndex, uint32_t currentFrame);
		//void RecordComputeCommanedBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex, uint32_t currentFrame);
		void RecordComputeCommandBuffer(VkCommandBuffer cmd, uint32_t frameIndex);
		void BuildAffectedTilesCPU(const std::vector<glm::vec2>& hitPositionsW, const std::vector<float>& radiiW, const std::vector<uint32_t>& damagesW, const std::vector<uint32_t>& candidateSlots, float pixelSizeWorld, int tileW, int tileH, std::vector<AffectedTile>& outTiles);
		void RecordEffectComputeCommandBuffer(VkCommandBuffer cmdBuf, uint32_t currentFrame);
		void RecordLineCommanedBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex, uint32_t currentFrame);
		void ConsumeDestructibleQueue(VkCommandBuffer uploadCB, uint32_t frameIndex);
		void ConsumeAnimationQueue(uint32_t frameIndex);
		//void AllocateCommandBuffers(VkDevice device, VkCommandPool commandPool);
		void CreateSyncObjects();


		void TransitionImageLayout(VkCommandBuffer cmd, VkImage image, VkImageLayout oldLayout, VkImageLayout newLayout);

	public:
		static PlayerData s_PlayerData;


	private:

		// holds pipeline for game and present
		Ref<VulkanGraphicsPipeline> m_vulkanGraphicsPipelines;
		Ref<VulkanFogOfWarPipelines> m_vulkanFogOfWarPipelines;

		

		VulkanContext* m_vulkanContext;
		VkSwapchainKHR m_swapchain;
		VkExtent2D m_swapchainExtent;
		VkDevice m_device;

		std::vector<VkSemaphore> m_imageAvailableSemaphores;
		std::vector<VkSemaphore> m_renderFinishedSemaphores;
		std::vector<VkFence> m_inFlightFences;


		VkFence m_computeFence;


		std::vector<VkDescriptorSet> m_gameViewportDescriptorSets;


		Ref<OrthographicCamera> m_camera;
		uint32_t m_imageIndex;
		uint32_t m_firstIndex = 0;
		uint32_t m_vertexOffset = 0;

		// effects shader
		std::vector<glm::vec2> m_hitsW;
		std::vector<float>     m_radiiW;
		std::vector<uint32_t>  m_damages;
		CollisionResultBuffer* m_collisionMapped[MAX_FRAMES_IN_FLIGHT] = { nullptr };

		std::vector<uint32_t> m_activeSlots = {};
		std::vector<uint32_t> m_collisionSlotsLastFrame;

		static VulkanRenderer2DData s_VulkanData;
		static VulkanBindlessRenderer2DData s_VulkanBindlessData;
		static VulkanRenderer2DTileDestructionData s_VulkanTilesToDestroyData;
		static CollisionData s_CollisionData;
		static EffectPushConstants s_effectPushConstants;
		static CPUExplosionData s_CPUExplosionsData;
		//static const uint32_t MaxTextures = 10;
		//std::array<CollisionTexture, MaxTextures> s_CollisionTextures;
		//CollisionTexture s_CollisionTextures;
		Ref<VulkanTexture> m_dummyTexture;

		static Ref<VulkanBindlessDescriptorSetRenderer> s_bindlessDescitproRenderer;
		Ref<VulkanUIRenderer> m_uiRenderer;


		float m_timer = 0.0f;
	

		Ref<VulkanShadowMap> m_shadowMap;


};



}
