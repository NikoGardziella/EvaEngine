#pragma once
#include <vulkan/vulkan.h>
#include "VulkanShader.h"
#include "VulkanTexture.h"
#include <Engine/Core/Core.h>



namespace Engine {

    class VulkanFogOfWarPipelines
    {
    public:


        struct FogDrawRange
        {
            uint32_t firstVertex = 0;
            uint32_t vertexCount = 0;
        };

        struct FogSubmitResult
        {
            FogDrawRange fan;
            FogDrawRange quad;
        };

        struct FogVertex
        {
            glm::vec2 pos;
        };

        struct alignas(16) FogPC {
            glm::mat4 uVP;
            glm::mat4 uInvVP;
            glm::vec2 mapMin;
            glm::vec2 mapSize;
            float time;
            uint32_t flags;
        };
        //static_assert(sizeof(FogPC) == 80, "FogPC push constant size must match GLSL");

        struct VulkanFogOfWarPipelinesCreateInfo
        {
            VkDevice device = VK_NULL_HANDLE;
            VkRenderPass renderPass = VK_NULL_HANDLE;
            VkExtent2D extent{};
            VkSampleCountFlagBits msaaSamples = VK_SAMPLE_COUNT_1_BIT;

       
        };

        bool Init(const VulkanFogOfWarPipelinesCreateInfo& ci);
       
        void Destroy();

        VkPipeline GetStencilWritePipeline() const { return m_pipeStencilWrite; }
        VkPipeline GetFogOverlayPipeline() const { return m_pipeFogOverlay; }
        VkPipelineLayout GetFogOverlayPipelineLayout() const { return m_pipelineLayout; }
        VkPipelineLayout GetVisibilityPipelineLayout() const { return m_visibilityPipelineLayout; }
        VkPipeline GetVisibilityPipeline() const { return m_pipeVisibilityMap; }

        const Ref<VulkanTexture>& GetFogOfWarTexture() const { return m_fogOfwarTexture; }
        VkFramebuffer GetVisibilityFramebuffer() const { return m_visibilityFramebuffer; }
        VkRenderPass GetVisibilityRenderPass() const { return m_visibilityRenderPass; }
        VkDescriptorSet GetVisibilityDescriptorSet() const { return m_descriptorSet; }


        void UpdateDescriptorSet();

    private:

        bool CreateOverlayPipeline(const VulkanFogOfWarPipelinesCreateInfo& ci);
        bool CreateVisibilityPipeline(const VulkanFogOfWarPipelinesCreateInfo& ci);

        void CreateFogPipelineLayouts(VkDevice device);
        void CreateDescriptorPool();
        void CreateVisibilityResources();

    private:

        VkDevice m_device = VK_NULL_HANDLE;
        VkPipeline m_pipeStencilWrite = VK_NULL_HANDLE;
        VkPipeline m_pipeFogOverlay = VK_NULL_HANDLE;
        VkPipeline m_pipeVisibilityMap = VK_NULL_HANDLE;
        VkShaderModule m_vs = VK_NULL_HANDLE;
        VkShaderModule m_fs = VK_NULL_HANDLE;

        Ref<VulkanShader> m_vulkanFogOfWarShader;
        Ref<VulkanShader> m_vulkanFogOfWarWriterShader;

        VkPipelineLayout m_pipelineLayout;

        Ref<VulkanTexture> m_fogOfwarTexture;

        VkDescriptorSetLayout m_descriptorSetLayout;
        VkPipelineLayout m_visibilityPipelineLayout;

        VkFramebuffer m_visibilityFramebuffer;
        VkRenderPass m_renderPass;
        VkRenderPass m_visibilityRenderPass;



        VkDescriptorSet m_descriptorSet = VK_NULL_HANDLE;
        VkDescriptorPool m_descriptorPool;
    };



    
}

