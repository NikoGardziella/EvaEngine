#pragma once

#include "Engine/Core/Core.h"
#include <vulkan/vulkan.h>
#include <glm/glm.hpp>

namespace Engine {

    class VulkanShader;

    class VulkanShadowGraphicsPipeline
    {
    public:
        VulkanShadowGraphicsPipeline() = default;
        ~VulkanShadowGraphicsPipeline() = default;

        // Initialize shadow pipelines for different geometry types
        void Init(VkDevice device, VkRenderPass shadowRenderPass);
        void Shutdown(VkDevice device);

        // Create shadow pipelines for each renderer type
        void Create3DShadowPipeline(VkDevice device, VkRenderPass shadowRenderPass, VkDescriptorSetLayout set0Layout);

        void CreateTilesShadowPipeline(VkDevice device, VkRenderPass shadowRenderPass,
            VkDescriptorSetLayout instanceLayout);

        void CreateGroundShadowPipeline(VkDevice device, VkRenderPass shadowRenderPass);

        // Getters
        VkPipeline Get3DShadowPipeline() const { return m_3dShadowPipeline; }
        VkPipeline GetTilesShadowPipeline() const { return m_tilesShadowPipeline; }
        VkPipeline GetGroundShadowPipeline() const { return m_groundShadowPipeline; }

        VkPipelineLayout Get3DShadowPipelineLayout() const { return m_3dShadowPipelineLayout; }
        VkPipelineLayout GetTilesShadowPipelineLayout() const { return m_tilesShadowPipelineLayout; }
        VkPipelineLayout GetGroundShadowPipelineLayout() const { return m_groundShadowPipelineLayout; }

    private:
        VkDevice m_device = VK_NULL_HANDLE;

        // 3D mesh shadow pipeline
        VkPipeline m_3dShadowPipeline = VK_NULL_HANDLE;
        VkPipelineLayout m_3dShadowPipelineLayout = VK_NULL_HANDLE;
        Ref<VulkanShader> m_3dShadowShader;

        // Tiles shadow pipeline
        VkPipeline m_tilesShadowPipeline = VK_NULL_HANDLE;
        VkPipelineLayout m_tilesShadowPipelineLayout = VK_NULL_HANDLE;
        Ref<VulkanShader> m_tilesShadowShader;

        // Ground shadow pipeline
        VkPipeline m_groundShadowPipeline = VK_NULL_HANDLE;
        VkPipelineLayout m_groundShadowPipelineLayout = VK_NULL_HANDLE;
        Ref<VulkanShader> m_groundShadowShader;

        void CreateDepthOnlyPipeline(VkDevice device,VkRenderPass renderPass, VkShaderModule vertShader, const VkPipelineVertexInputStateCreateInfo& vertexInputInfo,
            VkPipelineLayout pipelineLayout, VkPipeline& outPipeline
        );
    };

}