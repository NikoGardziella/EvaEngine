#pragma once

#include "VulkanBuffer.h"
#include "VulkanShader.h"
#include "VulkanTexture.h"
#include "Engine/Renderer/Texture.h"
#include "Engine/Platform/Vulkan/VulkanContext.h"
#include "Pixel/VulkanPixelTexture.h"

#include <vulkan/vulkan.h>
#include <vector>
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
        VulkanGraphicsPipeline(VulkanContext& vulkanContext);
        ~VulkanGraphicsPipeline();

        void UpdatePresentDescriptorSet(uint32_t imageIndex);
        void UpdateTrackedImageDescriptorSets(size_t frameIndex, const std::array<Ref<VulkanTexture>, 32>& textures);
        void UpdateCameraUBODescriptorSets();
        void UpdateUniformBuffer(uint32_t currentFrame, const glm::mat4& viewProjectionMatrix);

        VkPipeline GetGamePipeline() const { return m_gameGraphicsPipeline; }
        VkPipeline GetPresentPipeline() const { return m_presentPipeline; }
        VkPipelineLayout GetGamePipelineLayout() const { return m_gamePipelineLayout; }
        VkPipelineLayout GetPresentPipelineLayout() const { return m_presentPipelineLayout; }
        VkDescriptorSet GetGameDescriptorSet(size_t frameIndex) { return m_gameDescriptorSets[frameIndex]; }
        VkDescriptorSet GetCameraDescriptorSet(size_t frameIndex) { return m_cameraDescriptorSets[frameIndex]; }
        VkDescriptorSet GetPresentDescriptorSet(size_t frameIndex) { return m_presentDescriptorSets[frameIndex]; }
        VkSampler& GetPresentSampler() { return m_presentSampler; }

    private:

        void CreateGameGraphicsPipeline(VkRenderPass renderPass);
        void CreatePresentGraphicsPipeline(VkRenderPass renderPass);
        void CreatePresentPipelineLayout();
        void CreateDescriptorSetLayouts();
        void CreatePresentGameDescriptorPool();
        void CreateGameAndPresentDescriptorSets();
        void CreateGameDescriptorSet();
        void CreatePresentDescriptorSet();
		void CreateDescriptorSetLayout();
        void CreatePresentSampler();
        void CreateCameraDescriptorSetLayout();
        void CreateCameraDescriptorSet();

    private:

        VkExtent2D m_swapchainExtent;
        VkDevice m_device;
        VkPipeline m_gameGraphicsPipeline;
        VkPipeline m_presentPipeline;
        VkPipelineLayout m_gamePipelineLayout;
        VkPipelineLayout m_imguiPipelineLayout;
        VkPipelineLayout m_presentPipelineLayout;

        VkDescriptorSetLayout m_gameDescriptorSetLayout;
        VkDescriptorSetLayout m_presentDescriptorSetLayout;
        VkDescriptorSetLayout m_cameraDescriptorSetLayout;
        VkDescriptorPool m_presentGamedescriptorPool;
        VkDescriptorSet m_gameDescriptorSet;
        VkDescriptorSet m_presentDescriptorSet;

        std::vector<VkDescriptorSet> m_gameDescriptorSets;
        std::vector<VkDescriptorSet> m_cameraDescriptorSets;
        std::vector<VkDescriptorSet> m_presentDescriptorSets;
        VkDescriptorPool m_descriptorPool;
        std::vector<VulkanBuffer> m_uniformBuffers;

        Ref<VulkanShader> m_pixelGameShader;
        Ref<VulkanShader> m_fullscreenShader;
        Ref<VulkanShader> m_vulkanRenderShader;
        VkSampler m_presentSampler;

        std::vector<VkDynamicState> m_dynamicStates =
        {
            VK_DYNAMIC_STATE_VIEWPORT,
            VK_DYNAMIC_STATE_SCISSOR
        };

    };

}


