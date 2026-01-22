#pragma once
#include <vulkan/vulkan.h>
#include "VulkanShader.h"



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
            glm::vec2 playerPos;
            float visRadius;
            float time;
            uint32_t flags;
            uint32_t _pad0; // pad to 16-byte multiple
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


    private:
        VkPipelineLayout CreateFogPipelineLayout(VkDevice device);

    private:

        VkDevice m_device = VK_NULL_HANDLE;
        VkPipeline m_pipeStencilWrite = VK_NULL_HANDLE;
        VkPipeline m_pipeFogOverlay = VK_NULL_HANDLE;
        VkShaderModule m_vs = VK_NULL_HANDLE;
        VkShaderModule m_fs = VK_NULL_HANDLE;

        Ref<VulkanShader> m_vulkanFogOfWarShader;

        VkPipelineLayout m_pipelineLayout;
    };

}

