#pragma once
#include <vulkan/vulkan.h>
#include "VulkanShader.h"
#include "Engine/Core/Core.h"


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

        struct FogPC {
            glm::mat4 uVP;        // 64 bytes (offset 0)
            glm::vec2 playerPos;  // 8 bytes  (offset 64)
            float visRadius;      // 4 bytes  (offset 72)
            float time;           // 4 bytes  (offset 76)
            uint32_t flags;
            // Total: 80 bytes
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

