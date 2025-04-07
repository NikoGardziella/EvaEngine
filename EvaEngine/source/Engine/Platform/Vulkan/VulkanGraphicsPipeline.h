#pragma once

#include <vulkan/vulkan.h>
#include <vector>
#include "VulkanBuffer.h"
#include <Engine/Renderer/Texture.h>
#include "VulkanTexture.h"
#include <glm/glm.hpp>


namespace Engine {

    struct VulkanQuadVertex
    {
        glm::vec3 Position;  // Vertex position (x, y, z)
        glm::vec4 Color;     // Vertex color (r, g, b, a)
        glm::vec2 TexCoord;  // Texture coordinates (u, v)
        float TexIndex;      // Texture index for binding
        float TilingFactor;  // Tiling factor for the texture
    };

    class VulkanGraphicsPipeline
    {
       
    public:
        VulkanGraphicsPipeline(VkDevice device, VkExtent2D swapchainExtent, VkRenderPass renderPass);
        ~VulkanGraphicsPipeline();

        void CreateGraphicsPipeline(VkExtent2D swapchainExtent, VkRenderPass renderPass);

        void CreateDescriptorSet();

        void UpdateDescriptorSet(size_t frameIndex);

        VkPipeline GetPipeline() const { return m_graphicsPipeline; }
        VkPipelineLayout GetPipelineLayout() const { return m_pipelineLayout; }
		VkDescriptorSetLayout GetDescriptorSetLayout() const { return m_descriptorSetLayout; }
        VkDescriptorSet GetDescriptorSet(size_t frameIndex) const { return m_descriptorSets[frameIndex]; }
    private:
        void CreateShaderModule(const std::vector<char>& code, VkShaderModule* shaderModule);
		void CreateDescriptorSetLayout();


    private:
        VkDevice m_device;
        VkPipeline m_graphicsPipeline;
        VkPipelineLayout m_pipelineLayout;
        VkDescriptorSetLayout m_descriptorSetLayout;
        std::vector<VkDescriptorSet> m_descriptorSets;
        VkDescriptorPool m_descriptorPool;
        VulkanBuffer m_uniformBuffer;

		Ref<VulkanTexture> m_texture;

        //VkImageView m_textureImageView;
        //VkSampler m_textureSampler;

        std::vector<VkDynamicState> m_dynamicStates =
        {
            VK_DYNAMIC_STATE_VIEWPORT,
            VK_DYNAMIC_STATE_SCISSOR
        };

    };

}


