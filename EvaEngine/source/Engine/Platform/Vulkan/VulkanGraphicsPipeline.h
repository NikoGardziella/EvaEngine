#pragma once

#include <vulkan/vulkan.h>
#include <vector>

namespace Engine {

    class VulkanGraphicsPipeline
    {
       
    public:
        VulkanGraphicsPipeline(VkDevice device, VkExtent2D swapchainExtent, VkRenderPass renderPass);
        ~VulkanGraphicsPipeline();

        void CreateGraphicsPipeline(VkExtent2D swapchainExtent, VkRenderPass renderPass);

        VkPipeline GetPipeline() const { return m_graphicsPipeline; }
        VkPipelineLayout GetPipelineLayout() const { return m_pipelineLayout; }

    private:
        void CreateShaderModule(const std::vector<char>& code, VkShaderModule* shaderModule);

    private:
        VkDevice m_device;
        VkPipeline m_graphicsPipeline;
        VkPipelineLayout m_pipelineLayout;
        VkDescriptorSetLayout m_descriptorSetLayout;


        std::vector<VkDynamicState> m_dynamicStates =
        {
            VK_DYNAMIC_STATE_VIEWPORT,
            VK_DYNAMIC_STATE_SCISSOR
        };

    };

}


