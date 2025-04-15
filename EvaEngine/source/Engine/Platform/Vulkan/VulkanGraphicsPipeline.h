#pragma once

#include <vulkan/vulkan.h>
#include <vector>
#include "VulkanBuffer.h"
#include <Engine/Renderer/Texture.h>
#include "VulkanTexture.h"
#include <glm/glm.hpp>
#include "VulkanShader.h"


namespace Engine {

    struct VulkanQuadVertex
    {
        glm::vec3 Position;  // Vertex position (x, y, z)
        glm::vec4 Color;     // Vertex color (r, g, b, a)
        glm::vec2 TexCoord;  // Texture coordinates (u, v)
        float TexIndex;      // Texture index for binding
        float TilingFactor;  // Tiling factor for the texture
    };

    struct alignas(16) UniformBufferObject
    {
        glm::mat4 u_ViewProjection;
    };

    class VulkanGraphicsPipeline
    {
       
    public:
        VulkanGraphicsPipeline(VkDevice device, VkExtent2D swapchainExtent, VkRenderPass renderPass, Ref<VulkanShader> shader);
        ~VulkanGraphicsPipeline();

        void CreateGraphicsPipeline(VkExtent2D swapchainExtent, VkRenderPass renderPass);

        void CreateDescriptorSet();

        void UpdateDescriptorSets(size_t frameIndex);

        void UpdateDescriptorSets(uint32_t slotIndex, const Ref<VulkanTexture>& texture);

        void UpdateUniformBuffer(const glm::mat4& viewProjectionMatrix);
        //void BindTextures(VkCommandBuffer commandBuffer);


        VkPipeline GetPipeline() const { return m_graphicsPipeline; }
        VkPipelineLayout GetPipelineLayout() const { return m_pipelineLayout; }
		VkDescriptorSetLayout GetDescriptorSetLayout() const { return m_descriptorSetLayout; }
        VkDescriptorSet GetDescriptorSet(size_t frameIndex) { return m_descriptorSets[frameIndex]; }
    private:
		void CreateDescriptorSetLayout();

    private:
        VkDevice m_device;
        VkPipeline m_graphicsPipeline;
        VkPipelineLayout m_pipelineLayout;
        VkDescriptorSetLayout m_descriptorSetLayout;
        std::vector<VkDescriptorSet> m_descriptorSets;
        VkDescriptorPool m_descriptorPool;
        VulkanBuffer m_uniformBuffer;

        VkDescriptorSet m_playButtondescriptorSet;

		Ref<VulkanTexture> m_texture;
		//std::vector<Ref<VulkanTexture>> m_textures;

        Ref<VulkanShader> m_circleShader;
        //VkImageView m_textureImageView;
        //VkSampler m_textureSampler;

        std::vector<VkDynamicState> m_dynamicStates =
        {
            VK_DYNAMIC_STATE_VIEWPORT,
            VK_DYNAMIC_STATE_SCISSOR
        };

    };

}


