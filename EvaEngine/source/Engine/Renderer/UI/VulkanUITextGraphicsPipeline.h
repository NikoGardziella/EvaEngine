#pragma once
#include <vulkan/vulkan.h>


#include <array>
#include <vector>
#include <string>
#include <cstdint>
#include <Engine/Core/Core.h>
#include <Engine/Platform/Vulkan/VulkanTexture.h>
#include <Engine/Platform/Vulkan/VulkanShader.h>


class VulkanDevice;
class VulkanBuffer;
class VulkanTexture;

namespace Engine {

    class VulkanUITextGraphicsPipeline
    {
    public:
        
     

    public:
        struct UIInitConfig
        {
            VkRenderPass renderPass = VK_NULL_HANDLE;
            uint32_t subpassIndex = 0;

            uint32_t maxTextures = MAX_UI_TEXTURES;

            uint32_t viewportWidth = 1;
            uint32_t viewportHeight = 1;
        };

        struct CameraUBO
        {
            glm::mat4 ViewProjection;
        };

    public:
        VulkanUITextGraphicsPipeline(VkDevice device, VkPhysicalDevice physicalDevice);
        ~VulkanUITextGraphicsPipeline();

        VulkanUITextGraphicsPipeline(const VulkanUITextGraphicsPipeline&) = delete;
        VulkanUITextGraphicsPipeline& operator=(const VulkanUITextGraphicsPipeline&) = delete;

        void Init(const UIInitConfig& cfg, uint32_t framesInFlight);
        void Shutdown();

        VkDescriptorSet GetCameraDescriptorSet(uint32_t frame) const;
        VkDescriptorSet GetUITextureDescriptorSet(uint32_t frame) const;

        void UpdateCameraUBO(uint32_t frame, const CameraUBO& data);

        void UpdateUITextures(uint32_t frame, const std::vector<Ref<VulkanTexture>>& textures);

        VkPipeline GetPipeline() const { return m_pipeline; }
        VkPipelineLayout GetPipelineLayout() const { return m_pipelineLayout; }

        VkDescriptorSetLayout GetSetLayoutCamera() const { return m_setLayoutCamera; }
        VkDescriptorSetLayout GetSetLayoutUITextures() const { return m_setLayoutUITextures; }

    private:
        void CreateDescriptorSetLayouts();
        void CreateDescriptorPool(uint32_t framesInFlight);
        void AllocateDescriptorSets(uint32_t framesInFlight);

        void CreateCameraBuffers(uint32_t framesInFlight);
        void DestroyCameraBuffers();

        void CreatePipelineLayout();
        void CreatePipeline(const UIInitConfig& cfg);


        void FillVertexInputState(VkPipelineVertexInputStateCreateInfo& vi, std::array<VkVertexInputBindingDescription, 1>& bindings,
            std::array<VkVertexInputAttributeDescription, 4>& attrs) const;

    private:
        VkDevice m_device = VK_NULL_HANDLE;
        VkPhysicalDevice m_physicalDevice = VK_NULL_HANDLE;

        UIInitConfig m_cfg{};
        uint32_t m_framesInFlight = 0;

        // Descriptors
        VkDescriptorSetLayout m_setLayoutCamera = VK_NULL_HANDLE;    // set 0
        VkDescriptorSetLayout m_setLayoutUITextures = VK_NULL_HANDLE; // set 1

        VkDescriptorPool m_descriptorPool = VK_NULL_HANDLE;

        std::vector<VkDescriptorSet> m_setCamera;
        std::vector<VkDescriptorSet> m_setUITextures;

        // Camera UBO buffers (one per frame)
        struct CameraBuffer
        {
            VkBuffer buffer = VK_NULL_HANDLE;
            VkDeviceMemory memory = VK_NULL_HANDLE;
            void* mapped = nullptr;
            VkDeviceSize size = 0;
        };
        std::vector<CameraBuffer> m_cameraBuffers;

        VkPipelineLayout m_pipelineLayout = VK_NULL_HANDLE;
        VkPipeline m_pipeline = VK_NULL_HANDLE;


        Ref<VulkanShader> m_uiTextShader = VK_NULL_HANDLE;


    private:

        uint32_t FindMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties) const;
        void CreateBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& outBuffer, VkDeviceMemory& outMemory, void** outMapped);
        void DestroyBuffer(VkBuffer& buffer, VkDeviceMemory& memory, void** mapped);
    };
}

