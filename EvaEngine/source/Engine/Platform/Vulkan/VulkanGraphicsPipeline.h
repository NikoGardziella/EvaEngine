#pragma once
#include "Engine/Core/Core.h"
#include "VulkanBuffer.h"
#include "VulkanShader.h"
#include "VulkanTexture.h"
#include "Engine/Platform/Vulkan/VulkanContext.h"
#include "Pixel/VulkanPixelTexture.h"

#include <vulkan/vulkan.h>
#include <vector>
#include <glm/glm.hpp>


namespace Engine {


    struct CollisionResult
    {
        uint32_t collisionDetected = 0;
        uint32_t hitProjectileID_Low;
        uint32_t hitProjectileID_High;
        uint32_t _padding0 = 0; // Pad to 16-byte boundary

        glm::vec2 CollisionPosition;
        uint32_t _padding1 = 0;
        uint32_t _padding2 = 0;

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


    struct CollisionEntitiesGPU {
        glm::vec2 Position;    // 8 bytes
        float Radius;          // 4 bytes
        uint32_t Type;         // 4 bytes

        uint32_t ID_Low;       // 4 bytes
        uint32_t ID_High;      // 4 bytes

        glm::vec2 Size;        // 8 bytes (width, height of box)
        float Rotation;        // 4 bytes (in radians)

        uint32_t padding;      // 4 bytes to maintain 16-byte alignment (optional)
    };

    static_assert(sizeof(CollisionEntitiesGPU) == 40, "ProjectileGPU must be 32 bytes (std140 alignment)");

   

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
        void UpdateComputeDescriptorSet(uint32_t frameIndex, std::array<Ref<VulkanTexture>, MAX_TEXTURES>  input);
        void UpdateTrackedImageDescriptorSets(size_t frameIndex, const std::array<Ref<VulkanTexture>, MAX_TEXTURES>& textures);
        void UpdateProjectileDescriptorSets(size_t frameIndex, const std::array<Ref<VulkanTexture>, MAX_PROJECTILES>& textures);
        void UpdateTrackedImageDescriptorSets(size_t frameIndex, const std::vector<Ref<VulkanTexture>>& textures);

        void UpdateCameraUBODescriptorSets();
        void UpdateBulletUBODescriptorSets();
        void UpdateTextureInfoDescriptorSets();
        void UpdateCameraUniformBuffer(uint32_t currentFrame, const glm::mat4& viewProjectionMatrix);

        void UpdateCollisionUniformBuffer(uint32_t currentFrame, const std::array<CollisionEntitiesGPU, MAX_COLLISION_ENTITIES> bulletPositions);

        void UpdateTextureUniformBuffer(uint32_t currentFrame, const glm::ivec2& textureSize);



        VkPipeline GetGamePipeline() const { return m_gameGraphicsPipeline; }
        VkPipeline GetPresentPipeline() const { return m_presentPipeline; }
		VkPipeline GetLinePipeline() const { return m_linePipeline; }
		VkPipeline GetComputePipeline() const { return m_computePipeline; }
		VkPipeline GetProjectilePipeline() const { return m_projectilePipeline; }

        VkPipelineLayout GetGamePipelineLayout() const { return m_gamePipelineLayout; }
        VkPipelineLayout GetPresentPipelineLayout() const { return m_presentPipelineLayout; }
		VkPipelineLayout GetLinePipelineLayout() const { return m_linePipelineLayout; }
		VkPipelineLayout GetComputePipelineLayout() const { return m_computePipelineLayout; }
		VkPipelineLayout GetProjectilePipelineLayout() const { return m_projectilePipelineLayout; }
        
        VkDescriptorSet GetGameDescriptorSet(size_t frameIndex) { return m_gameDescriptorSets[frameIndex]; }
        VkDescriptorSet GetCameraDescriptorSet(size_t frameIndex) { return m_cameraDescriptorSets[frameIndex]; }
        VkDescriptorSet GetPresentDescriptorSet(size_t frameIndex) { return m_presentDescriptorSets[frameIndex]; }
		VkDescriptorSet GetComputeDescriptorSet(size_t frameIndex) { return m_computeDescriptorSet[frameIndex]; }
		VkDescriptorSet GetProjectileDescriptorSet(size_t frameIndex) { return m_projectileDescriptorSet[frameIndex]; }
        VkDescriptorSet GetLineDescriptorSet(size_t frameIndex) { return m_lineDescriptorSet[frameIndex]; }

        VkSampler& GetPresentSampler() { return m_presentSampler; }

        VulkanBuffer GetBulletUniformBuffer(uint32_t imageIndex) { return m_bulletUniformBuffers[imageIndex]; }
        VulkanBuffer GetTextureInfoUniformBuffer(uint32_t imageIndex) { return m_textureUniformBuffers[imageIndex]; }

        VkBuffer GetGPUCollisionBuffer() const { return m_GPUCollisionresultBufferBuffer; }
        VkDeviceMemory GetGPUCollisionMemory() const { return m_GPUCollisionresultBufferMemory; }


       // StorageImage GetOutputImage() { return m_outputTextureImage; }

    private:

        void CreateGameGraphicsPipeline(VkRenderPass renderPass);
        void CreateLineGraphicsPipeline(VkRenderPass renderPass);
        void CreateComputeGraphicsPipeline(VkRenderPass renderPass);
        void CreatePresentGraphicsPipeline(VkRenderPass renderPass);
        void CreateProjectileGraphicsPipeline(VkRenderPass renderPass);
        void CreatePresentPipelineLayout();
        void CreateDescriptorSetLayouts();
        void CreateComputeDescriptorSetLayout();
        void CreateComputeArrayDescriptorSetLayout();
        void CreateProjectileDescriptorSetLayout();
        void CreatePresentGameDescriptorPool();
        void CreateGameDescriptorSet();
        void CreateProjectileDescriptorSet();
        void CreateLineDescriptorSet();
        void CreatePresentDescriptorSet();
        void CreatePresentSampler();
        void CreateCameraDescriptorSetLayout();
        void CreateCameraDescriptorSet();
        void CreateComputeDescriptorSet();
        void CreateGPUCollisionResultBuffer();
        StorageImage CreateStorageImage(VkDevice device, VkPhysicalDevice physicalDevice, uint32_t width, uint32_t height, VkFormat format, VkCommandPool commandPool, VkQueue graphicsQueue);

    private:

        VkExtent2D m_swapchainExtent;
        VkDevice m_device;
        VkPipeline m_gameGraphicsPipeline;
        VkPipeline m_presentPipeline;
        VkPipeline m_linePipeline;
        VkPipeline m_computePipeline;
        VkPipeline m_projectilePipeline;
        VkPipelineLayout m_gamePipelineLayout;
        VkPipelineLayout m_projectilePipelineLayout;
        VkPipelineLayout m_linePipelineLayout;
        VkPipelineLayout m_imguiPipelineLayout;
        VkPipelineLayout m_presentPipelineLayout;
        VkPipelineLayout m_computePipelineLayout;

        VkDescriptorSetLayout m_gameDescriptorSetLayout;
        VkDescriptorSetLayout m_presentDescriptorSetLayout;
        VkDescriptorSetLayout m_lineDescriptorSetLayout;
        VkDescriptorSetLayout m_projectileDescriptorSetLayout;
        VkDescriptorSetLayout m_cameraDescriptorSetLayout;
        VkDescriptorSetLayout m_computeDescriptorSetLayout;
        VkDescriptorSetLayout m_computeArrayDescriptorSetLayout;

        VkDescriptorPool m_presentGamedescriptorPool;

        std::vector<VkDescriptorSet> m_lineDescriptorSet;
        std::vector<VkDescriptorSet> m_projectileDescriptorSet;
        std::vector<VkDescriptorSet> m_computeDescriptorSet;
        std::vector<VkDescriptorSet> m_gameDescriptorSets;
        std::vector<VkDescriptorSet> m_cameraDescriptorSets;
        std::vector<VkDescriptorSet> m_presentDescriptorSets;
        VkDescriptorPool m_descriptorPool;
		VkDescriptorPool m_lineDescriptorPool;
        std::vector<VulkanBuffer> m_uniformBuffers;
        std::vector<VulkanBuffer> m_bulletUniformBuffers;
        std::vector<VulkanBuffer> m_textureUniformBuffers;

        Ref<VulkanShader> m_pixelGameShader;
        Ref<VulkanShader> m_fullscreenShader;
        Ref<VulkanShader> m_lineShader;
        Ref<VulkanShader> m_vulkanRenderShader;
        Ref<VulkanShader> m_vulkanProjectileRenderShader;
        Ref<VulkanShader> m_computeShader;
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

        VkBuffer m_healthBuffer;
        VkDeviceMemory m_healthBufferMemory;

        Ref<VulkanTexture> m_whiteTexture;
        Ref<VulkanTexture> m_dummyTexture;

    };

}


