#pragma once

#include "VulkanBuffer.h"
#include "VulkanShader.h"
#include "VulkanTexture.h"
#include "Engine/Platform/Vulkan/VulkanContext.h"
#include "Pixel/VulkanPixelTexture.h"

#include <vulkan/vulkan.h>
#include <vector>
#include <glm/glm.hpp>

namespace Engine {

    const int MAX_TEXTURE_DESCRIPTORS = 512;

    struct CollisionResult
    {
        uint32_t collisionDetected = 0;
        uint32_t hitProjectileID_Low;
        uint32_t hitProjectileID_High;
        uint32_t padding = 0;

        uint64_t GetProjectileID() const
        {
            return (uint64_t(hitProjectileID_High) << 32) | hitProjectileID_Low;
        }
    };
    static_assert(sizeof(CollisionResult) == 16, "CollisionResult must be 16 bytes (std140 alignment)");

    struct ProjectileGPU {
        glm::vec2 Position;         // 8 bytes
        float Radius;               // 4 bytes
        float padding0;             // 4 bytes (to align next field to 16)
        uint32_t ID_Low;            // 4 bytes
        uint32_t ID_High;           // 4 bytes
        uint32_t padding1[2];       // 8 bytes to align to 32-byte stride
    };
    static_assert(sizeof(ProjectileGPU) == 32, "ProjectileGPU must be 32 bytes (std140 alignment)");


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
        void UpdateDescriptorSet(uint32_t imageIndex, VkDescriptorSet descriptorset, VkImageView image); // remove?
        void UpdateComputeDescriptorSet(uint32_t frameIndex, std::vector<Ref<VulkanTexture>> input, std::vector<Ref<VulkanTexture>> output);
        void UpdateTrackedImageDescriptorSets(size_t frameIndex, const std::array<Ref<VulkanTexture>, 32>& textures);
        void UpdateTrackedImageDescriptorSets(size_t frameIndex, const std::vector<Ref<VulkanTexture>>& textures);
        void UpdateComputeArrayDescriptorSets(size_t frameIndex, const std::array<Ref<VulkanTexture>, 32>& textures);

        void UpdateCameraUBODescriptorSets();
        void UpdateBulletUBODescriptorSets();
        void UpdateTextureInfoDescriptorSets();
        void UpdateStorageImageDescriptorSets();
        void UpdateCameraUniformBuffer(uint32_t currentFrame, const glm::mat4& viewProjectionMatrix);

        void UpdateBulletUniformBuffer(uint32_t currentFrame, const std::array<ProjectileGPU, 32> bulletPositions);

        void UpdateTextureUniformBuffer(uint32_t currentFrame, const glm::ivec2& textureSize);



        VkPipeline GetGamePipeline() const { return m_gameGraphicsPipeline; }
        VkPipeline GetPresentPipeline() const { return m_presentPipeline; }
		VkPipeline GetLinePipeline() const { return m_linePipeline; }
		VkPipeline GetComputePipeline() const { return m_computePipeline; }
        VkPipelineLayout GetGamePipelineLayout() const { return m_gamePipelineLayout; }
        VkPipelineLayout GetPresentPipelineLayout() const { return m_presentPipelineLayout; }
		VkPipelineLayout GetLinePipelineLayout() const { return m_linePipelineLayout; }
		VkPipelineLayout GetComputePipelineLayout() const { return m_computePipelineLayout; }
        VkDescriptorSet GetGameDescriptorSet(size_t frameIndex) { return m_gameDescriptorSets[frameIndex]; }
        VkDescriptorSet GetCameraDescriptorSet(size_t frameIndex) { return m_cameraDescriptorSets[frameIndex]; }
        VkDescriptorSet GetPresentDescriptorSet(size_t frameIndex) { return m_presentDescriptorSets[frameIndex]; }
		VkDescriptorSet GetComputeDescriptorSet(size_t frameIndex) { return m_computeDescriptorSet[frameIndex]; }
		
        VkDescriptorSet GetLineDescriptorSet() { return m_lineDescriptorSet; }

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
        void CreatePresentPipelineLayout();
        void CreateDescriptorSetLayouts();
        void CreateComputeDescriptorSetLayout();
        void CreateComputeArrayDescriptorSetLayout();
        void CreatePresentGameDescriptorPool();
        void CreateGameDescriptorSet();
        void CreateLineDescriptorSet();
        void CreatePresentDescriptorSet();
		void CreateDescriptorSetLayout();
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
        VkPipelineLayout m_gamePipelineLayout;
        VkPipelineLayout m_linePipelineLayout;
        VkPipelineLayout m_imguiPipelineLayout;
        VkPipelineLayout m_presentPipelineLayout;
        VkPipelineLayout m_computePipelineLayout;

        VkDescriptorSetLayout m_gameDescriptorSetLayout;
        VkDescriptorSetLayout m_presentDescriptorSetLayout;
        VkDescriptorSetLayout m_lineDescriptorSetLayout;
        VkDescriptorSetLayout m_cameraDescriptorSetLayout;
        VkDescriptorSetLayout m_computeDescriptorSetLayout;
        VkDescriptorSetLayout m_computeArrayDescriptorSetLayout;

        VkDescriptorPool m_presentGamedescriptorPool;
        VkDescriptorSet m_lineDescriptorSet;

        std::vector<VkDescriptorSet> m_computeDescriptorSet;
        std::vector<VkDescriptorSet> m_gameDescriptorSets;
        std::vector<VkDescriptorSet> m_cameraDescriptorSets;
        std::vector<VkDescriptorSet> m_presentDescriptorSets;
        VkDescriptorPool m_descriptorPool;
        std::vector<VulkanBuffer> m_uniformBuffers;
        std::vector<VulkanBuffer> m_bulletUniformBuffers;
        std::vector<VulkanBuffer> m_textureUniformBuffers;
        VulkanBuffer m_pixelStagingBuffers;

        Ref<VulkanShader> m_pixelGameShader;
        Ref<VulkanShader> m_fullscreenShader;
        Ref<VulkanShader> m_lineShader;
        Ref<VulkanShader> m_vulkanRenderShader;
        Ref<VulkanShader> m_computeShader;
        VkSampler m_presentSampler;

        std::vector<VkDynamicState> m_dynamicStates =
        {
            VK_DYNAMIC_STATE_VIEWPORT,
            VK_DYNAMIC_STATE_SCISSOR
        };

        std::vector<BulletData> bullets;
        StorageImage m_storageImage;
        StorageImage  m_pixelTextureImage;
        StorageImage  m_outputTextureImage;


        VkBuffer m_GPUCollisionresultBufferBuffer;
        VkDeviceMemory  m_GPUCollisionresultBufferMemory;
    };

}


