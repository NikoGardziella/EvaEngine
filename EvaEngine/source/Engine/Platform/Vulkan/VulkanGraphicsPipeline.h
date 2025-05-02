#pragma once

#include <vulkan/vulkan.h>
#include <vector>
#include "VulkanBuffer.h"
#include <Engine/Renderer/Texture.h>
#include "VulkanTexture.h"
#include <glm/glm.hpp>
#include "VulkanShader.h"
#include "Pixel/VulkanPixelTexture.h"
#include "Engine/Platform/Vulkan/VulkanContext.h"

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
        VulkanGraphicsPipeline(VkDevice device, VkExtent2D swapchainExtent, VkRenderPass renderPass, VkRenderPass imGuirenderPass, Ref<VulkanShader> shader);
        ~VulkanGraphicsPipeline();

        void CreateGameGraphicsPipeline(VkExtent2D swapchainExtent, VkRenderPass renderPass);

        void CreateImGuiPipeline(VkRenderPass imGuiRenderPass);

        void CreateFullscreenGraphicsPipeline(VkRenderPass renderPass);


        void CreateImGuiPipelineLayout();
        void CreateFullscreenPipelineLayout();

        void CreateDescriptorSetLayouts(VulkanContext* context);

        void CreatePresentGameDescriptorPool(VulkanContext* context);

        void CreateGamePresentDescriptorSets(VulkanContext* context);

        void UpdateGamePresentDescriptorSets(VulkanContext* context, VkImageView swapchainImageView, VkSampler sampler);

        void CreateGameDescriptorSet();

        void CreatePresentDescriptorSet();

        void UpdatePresentDescriptorSet(uint32_t imageIndex);

        void UpdateTrackedImageDescriptorSets(size_t frameIndex, VkImageView imageView);

        void UpdateTrackedImageDescriptorSets(size_t frameIndex, const std::array<Ref<VulkanTexture>, 32>& textures);

        void CreatePresentSampler();

        void CreateCameraDescriptorSetLayout();

        void CreateCameraDescriptorSet();

        void UpdateCameraUBODescriptorSets();

        void UpdateGameDescriptorSets(size_t frameIndex);

        void UpdateGameDescriptorSets(uint32_t slotIndex, const Ref<VulkanTexture>& texture);

        void UpdateUniformBuffer(uint32_t currentFrame, const glm::mat4& viewProjectionMatrix);
        //void BindTextures(VkCommandBuffer commandBuffer);


        VkPipeline GetGamePipeline() const { return m_gameGraphicsPipeline; }
        VkPipeline GetFullscreenPipeline() const { return m_fullscreenPipeline; }
        VkPipelineLayout GetGamePipelineLayout() const { return m_gamePipelineLayout; }
        VkPipelineLayout GetFullscreenPipelineLayout() const { return m_fullscreenPipelineLayout; }
		VkDescriptorSetLayout GetDescriptorSetLayout() const { return m_descriptorSetLayout; }
        VkDescriptorSet GetGameDescriptorSet(size_t frameIndex) { return m_gameDescriptorSets[frameIndex]; }
        VkDescriptorSet GetCameraDescriptorSet(size_t frameIndex) { return m_cameraDescriptorSets[frameIndex]; }
        VkDescriptorSet GetPresentDescriptorSet(size_t frameIndex) { return m_presentDescriptorSets[frameIndex]; }
        VkSampler& GetPresentSampler() { return m_presentSampler; }

    private:
		void CreateDescriptorSetLayout();
        void CreateImGuiDescriptorSetLayout();
        void CreateFullscreenDescriptorSetLayout();

    private:
        VkDevice m_device;
        VkPipeline m_gameGraphicsPipeline;
        VkPipeline m_imguiPipeline;
        VkPipeline m_fullscreenPipeline;
        VkPipelineLayout m_gamePipelineLayout;
        VkPipelineLayout m_imguiPipelineLayout;
        VkPipelineLayout m_fullscreenPipelineLayout;
        VkDescriptorSetLayout m_descriptorSetLayout;
        VkDescriptorSetLayout m_imguiDescriptorSetLayout;
        VkDescriptorSetLayout m_fullscreenDescriptorSetLayout;


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

        VkDescriptorSet m_playButtondescriptorSet;

		Ref<VulkanTexture> m_texture;
        Ref<VulkanPixelTexture> m_pixelTexture;
        Ref<VulkanShader> m_pixelGameShader;
        Ref<VulkanShader> m_imGuiShader;
        Ref<VulkanShader> m_fullscreenShader;

		//std::vector<Ref<VulkanTexture>> m_textures;

        Ref<VulkanShader> m_vulkanRenderShader;
        VkSampler m_presentSampler;
        //VkImageView m_textureImageView;
        //VkSampler m_textureSampler;

        std::vector<VkDynamicState> m_dynamicStates =
        {
            VK_DYNAMIC_STATE_VIEWPORT,
            VK_DYNAMIC_STATE_SCISSOR
        };

    };

}


