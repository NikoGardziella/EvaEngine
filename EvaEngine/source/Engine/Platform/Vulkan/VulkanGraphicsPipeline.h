#pragma once
#include "Engine/Core/Core.h"
#include "VulkanBuffer.h"
#include "VulkanShader.h"
#include "VulkanTexture.h"
#include "Engine/Platform/Vulkan/VulkanContext.h"

#include <vulkan/vulkan.h>
#include <vector>
#include <glm/glm.hpp>
#include <cstddef>
#include <Engine/Map/TextureStreaming/TextureStreamingSystem.h>
#include "VulkanBindlessDescriptorSet.h"


namespace Engine {

    struct ExplosionEvent
    {
        glm::vec2 position;
        uint32_t radius;
    };

    struct CollisionResult
    {
        uint32_t collisionDetected = 0;
        uint32_t hitProjectileID_Low;
        uint32_t hitProjectileID_High;
        float    DestructionRadius = 0; // Pad to 16-byte boundary

        glm::vec2 CollisionPosition;
        uint32_t Health = 0;
        uint32_t Damage = 0;

        uint64_t GetProjectileID() const
        {
            return (uint64_t(hitProjectileID_High) << 32) | hitProjectileID_Low;
        }
    };
    static_assert(sizeof(CollisionResult) == 32, "CollisionResult must be 32 bytes (std140 alignment)");
    
    struct CollisionResultBuffer
    {
        uint32_t collisionCount;
        uint32_t _padding0;
        uint32_t _padding1;
        uint32_t _padding2;
        CollisionResult results[MAX_COLLISION_RESULTS];
    };

    struct CollisionPlayerEntitiesGPU {
        glm::vec2 Position;    // 8 bytes
        float ColliderRadius;  // 4 bytes

        uint32_t ID_Low;       // 4 bytes
        uint32_t ID_High;      // 4 bytes

    };


    // std430 mirror of the GLSL struct (exact byte layout)
    struct CollisionEntitiesGPU {
        glm::vec2 Position;          //  0..7
        float     ColliderRadius;    //  8..11
        uint32_t  Type;              // 12..15

        uint32_t  ID_Low;            // 16..19
        uint32_t  ID_High;           // 20..23

        glm::vec2 Size;              // 24..31
        float     Rotation;          // 32..35

        uint32_t  Damage;            // 36..39
        float     DestructionRadius; // 40..43

        uint32_t  _pad0;             // 44..47  <-- forces EndPos to 8B alignment

        glm::vec2 EndPos;            // 48..55
        glm::vec2 Dir;               // 56..63

        float     RayLen;            // 64..67
        float     Z1;                // 68..71
    };
    static_assert(sizeof(CollisionEntitiesGPU) == 72, "SSBO stride must be 72");
    static_assert(offsetof(CollisionEntitiesGPU, EndPos) == 48, "EndPos misaligned");
    static_assert(offsetof(CollisionEntitiesGPU, Dir) == 56, "Dir misaligned");
    static_assert(offsetof(CollisionEntitiesGPU, RayLen) == 64, "RayLen misaligned");
    static_assert(offsetof(CollisionEntitiesGPU, Z1) == 68, "Z1 misaligned");


    struct PlayerPC {
        glm::vec2 WindowOriginWorld;   // world units
        float     PixelSizeWorld;      // world units per pixel
        uint32_t  ChunkSizePixels;     // e.g. 4096
        uint32_t  NumPlayers;          // threads to run

    };
    static_assert(sizeof(PlayerPC) % 4 == 0, "push constants must be 4-byte aligned");


    struct BulletData
    {
        glm::vec2 Position;
    };

    struct TextureInfo
    {
        glm::ivec2 TextureSize;
    };


    struct VulkanQuadVertex
    {
        glm::vec3 Position;  // Vertex position (x, y, z)
        glm::vec4 Color;     // Vertex color (r, g, b, a)
        glm::vec2 TexCoord;  // Texture coordinates (u, v)
        float TexIndex;      // Texture index for binding
        float TilingFactor;  // Tiling factor for the texture
    };

    struct VulkanProjectileVertex
    {
        glm::vec3 Position;
        float _padding0;        // Align vec4

        glm::vec4 Color;

        glm::vec2 TexCoord;
        glm::vec2 _padding1;    // Align to vec4

        float TexIndex;
        float _padding2[3];     // pad to multiple of 16

        // Total size: 16 + 16 + 16 + 16 = 64 bytes
    };


    struct VulkanLineVertex
    {
        glm::vec3 Position;
        glm::vec4 Color;
        
    };

    struct StorageImage
    {
        VkImage Image;
        VkDeviceMemory Memory;
        VkImageView ImageView;
        uint32_t    Width;
        uint32_t    Height;

    };

  

    class VulkanGraphicsPipeline
    {
       
    public:
        VulkanGraphicsPipeline(VulkanContext& vulkanContext);
        ~VulkanGraphicsPipeline();

        void UpdatePresentDescriptorSet(uint32_t imageIndex);
        void UpdatePlayerCollisionDescriptorSet(uint32_t frameIndex,const  std::array<Ref<VulkanTexture>, CHUNK_GRID_SIZE> healthTextures);
        void UpdateProjectileDescriptorSets(size_t frameIndex, const std::array<Ref<VulkanTexture>, MAX_PROJECTILES>& textures);
       
        void UpdateGameDrawAndVisualImagesDescriptorSets(size_t frameIndex, const std::array<Ref<VulkanTexture>, MAX_TEXTURES>& textures, const std::array<Ref<VulkanTexture>, CHUNK_GRID_SIZE>& visualTextures);
        void UpdateTrackedImageDescriptorSets(size_t frameIndex, const std::vector<Ref<VulkanTexture>>& textures);


        void UpdateCameraUBODescriptorSets();
        void UpdateBulletUBODescriptorSets();
        void UpdateTextureInfoDescriptorSets();
        void UpdateCameraUniformBuffer(uint32_t currentFrame, const glm::mat4& viewProjectionMatrix);

        void UpdateCollisionUniformBuffer(uint32_t currentFrame, const std::array<CollisionEntitiesGPU, MAX_COLLISION_ENTITIES> bulletPositions);

        void UpdatePLayerCollisionUniformBuffer(uint32_t currentFrame, const std::array<CollisionPlayerEntitiesGPU, PLAYER_COUNT> collidingPlayerData);

        void UpdateTextureUniformBuffer(uint32_t currentFrame, const glm::ivec2& textureSize);




        VkPipeline GetGamePipeline() const { return m_gameGraphicsPipeline; }
        VkPipeline GetPresentPipeline() const { return m_presentPipeline; }
		VkPipeline GetLinePipeline() const { return m_linePipeline; }
		VkPipeline GetProjectilePipeline() const { return m_projectilePipeline; }
        VkPipeline GetPlayerCollisionComputePipeline() const { return m_playerCollisionPipeline; }


        VkPipelineLayout GetGamePipelineLayout() const { return m_gamePipelineLayout; }
        VkPipelineLayout GetPresentPipelineLayout() const { return m_presentPipelineLayout; }
		VkPipelineLayout GetLinePipelineLayout() const { return m_linePipelineLayout; }
		VkPipelineLayout GetProjectilePipelineLayout() const { return m_projectilePipelineLayout; }
        VkPipelineLayout GetPlayerCollisionComputePipelineLayout() const { return m_playerCollisionPipelineLayout; }


        VkDescriptorSet GetGameDescriptorSet(size_t frameIndex) { return m_gameDescriptorSets[frameIndex]; }
        VkDescriptorSet GetCameraDescriptorSet(size_t frameIndex) { return m_cameraDescriptorSets[frameIndex]; }
        VkDescriptorSet GetPresentDescriptorSet(size_t frameIndex) { return m_presentDescriptorSets[frameIndex]; }
		VkDescriptorSet GetProjectileDescriptorSet(size_t frameIndex) { return m_projectileDescriptorSet[frameIndex]; }
        VkDescriptorSet GetLineDescriptorSet(size_t frameIndex) { return m_lineDescriptorSet[frameIndex]; }
        VkDescriptorSet GetPlayerCollisionComputeDescriptorSet(size_t frameIndex) { return m_playerCollisionDescriptorSets[frameIndex]; }

        VkSampler& GetPresentSampler() { return m_presentSampler; }

        VulkanBuffer GetBulletUniformBuffer(uint32_t imageIndex) { return m_bulletUniformBuffers[imageIndex]; }
        VulkanBuffer GetPlayerCollisionUniformBuffer(uint32_t imageIndex) { return m_playerUniformBuffers[imageIndex]; }
        VulkanBuffer GetTextureInfoUniformBuffer(uint32_t imageIndex) { return m_textureUniformBuffers[imageIndex]; }

        VkBuffer GetGPUCollisionResultBuffer() const { return m_GPUCollisionresultBufferBuffer; }
        VkDeviceMemory GetGPUCollisionMemory() const { return m_GPUCollisionresultBufferMemory; }
        
        VkBuffer GetPLayerollisionBuffer() const { return m_playerCollisionresultBufferBuffer; }
        VkDeviceMemory GetPlayerCollisionMemory() const { return m_playerCollisionresultBufferMemory; }


        VkBuffer GetBlockedTileMaskBuffer() const { return m_blockedTileMaskBuffer; }
        VkDeviceMemory GetBlockedTileMaskMemory() const { return m_blockedTileMaskMemory;  }

        VkBuffer GetExplosionBuffer() const { return m_explosionBuffer; }
        VkDeviceSize GetExplosionBufferSize() const { return m_explosionBufferSize; }
        VkDeviceMemory GetEffectsBufferMemory() const { return m_explosionBufferMemory; }

       // StorageImage GetOutputImage() { return m_outputTextureImage; }



    private:
        void CreateGameGraphicsPipeline(VkRenderPass renderPass);
        void CreateLineGraphicsPipeline(VkRenderPass renderPass);
      
        void CreatePlayerCollisionDescriptorSetLayout();
        void CreatePlayerCollisionPipeline();
        void CreatePresentGraphicsPipeline(VkRenderPass renderPass);
        void CreateProjectileGraphicsPipeline(VkRenderPass renderPass);
        void CreatePresentPipelineLayout();
        void CreatePlayerCollisionPipelineLayout();
        void CreateDescriptorSetLayouts();
        void CreateProjectileDescriptorSetLayout();
        void CreatePresentGameDescriptorPool();
        void CreateGameDescriptorSet();
        void CreateProjectileDescriptorSet();
        void CreateLineDescriptorSet();
        void CreatePlayerCollisionDescriptorSets();
        void CreatePresentDescriptorSet();
        void CreatePresentSampler();
        void CreateCameraDescriptorSetLayout();
        void CreateCameraDescriptorSet();
        void CreatePlayerCollisionResultBuffer();
        void CreateGPUCollisionResultBuffer();
        void CreateBlockedTileMaskBuffer();
        void CreateExplosionBuffer();
        StorageImage CreateStorageImage(VkDevice device, VkPhysicalDevice physicalDevice, uint32_t width, uint32_t height, VkFormat format, VkCommandPool commandPool, VkQueue graphicsQueue);

    private:

        VkExtent2D m_swapchainExtent;
        VkDevice m_device;
        VkPipeline m_gameGraphicsPipeline;
        VkPipeline m_presentPipeline;
        VkPipeline m_linePipeline;
        VkPipeline m_projectilePipeline;
        VkPipeline m_playerCollisionPipeline;
        VkPipelineLayout m_gamePipelineLayout;
        VkPipelineLayout m_projectilePipelineLayout;
        VkPipelineLayout m_linePipelineLayout;
        VkPipelineLayout m_imguiPipelineLayout;
        VkPipelineLayout m_presentPipelineLayout;
        VkPipelineLayout m_playerCollisionPipelineLayout;

        VkDescriptorSetLayout m_gameDescriptorSetLayout;
        VkDescriptorSetLayout m_presentDescriptorSetLayout;
        VkDescriptorSetLayout m_lineDescriptorSetLayout;
        VkDescriptorSetLayout m_projectileDescriptorSetLayout;
        VkDescriptorSetLayout m_cameraDescriptorSetLayout;
        VkDescriptorSetLayout m_playerCollisionDescriptorSetLayout;

        VkDescriptorPool m_presentGamedescriptorPool;

        std::vector<VkDescriptorSet> m_lineDescriptorSet;
        std::vector<VkDescriptorSet> m_projectileDescriptorSet;
        std::vector<VkDescriptorSet> m_gameDescriptorSets;
        std::vector<VkDescriptorSet> m_cameraDescriptorSets;
        std::vector<VkDescriptorSet> m_presentDescriptorSets;
        std::vector<VkDescriptorSet> m_playerCollisionDescriptorSets;
        VkDescriptorPool m_descriptorPool;
		VkDescriptorPool m_lineDescriptorPool;
        std::vector<VulkanBuffer> m_uniformBuffers;
        std::vector<VulkanBuffer> m_bulletUniformBuffers;
        std::vector<VulkanBuffer> m_playerUniformBuffers;
        std::vector<VulkanBuffer> m_textureUniformBuffers;

        Ref<VulkanShader> m_pixelGameShader;
        Ref<VulkanShader> m_fullscreenShader;
        Ref<VulkanShader> m_lineShader;
        Ref<VulkanShader> m_vulkanRenderShader;
        Ref<VulkanShader> m_vulkanProjectileRenderShader;
        Ref<VulkanShader> m_playerCollisionComputeShader;
        VkSampler m_presentSampler;

        std::vector<VkDynamicState> m_dynamicStates =
        {
            VK_DYNAMIC_STATE_VIEWPORT,
            VK_DYNAMIC_STATE_SCISSOR
        };

        std::vector<BulletData> bullets;
       // StorageImage m_storageImage;
       // StorageImage  m_pixelTextureImage;
      // StorageImage  m_outputTextureImage;


        VkBuffer m_GPUCollisionresultBufferBuffer;
        VkDeviceMemory  m_GPUCollisionresultBufferMemory;

        VkBuffer m_playerCollisionresultBufferBuffer;
        VkDeviceMemory  m_playerCollisionresultBufferMemory;



        VkBuffer m_explosionBuffer;
        VkDeviceSize m_explosionBufferSize;
        VkDeviceMemory m_explosionBufferMemory;

        VkBuffer m_blockedTileMaskBuffer;
        VkDeviceMemory  m_blockedTileMaskMemory;

        VkBuffer m_healthBuffer;
        VkDeviceMemory m_healthBufferMemory;

        Ref<VulkanTexture> m_whiteTexture;
        Ref<VulkanTexture> m_dummyTexture;


    };

}


