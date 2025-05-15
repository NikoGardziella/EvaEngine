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

    struct VulkanLineVertex
    {
        glm::vec3 Position;
        glm::vec4 Color;
        //int EntityID; // 4 bytes, aligns fine after vec4 (16 bytes)
        /*
        static VkVertexInputBindingDescription GetBindingDescription()
        {
            VkVertexInputBindingDescription bindingDesc{};
            bindingDesc.binding = 0;
            bindingDesc.stride = sizeof(VulkanLineVertex);
            bindingDesc.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
            return bindingDesc;
        }

        static std::array<VkVertexInputAttributeDescription, 3> GetAttributeDescriptions()
        {
            std::array<VkVertexInputAttributeDescription, 3> attributeDescs{};

            attributeDescs[0].binding = 0;
            attributeDescs[0].location = 0;
            attributeDescs[0].format = VK_FORMAT_R32G32B32_SFLOAT;
            attributeDescs[0].offset = offsetof(VulkanLineVertex, Position);

            attributeDescs[1].binding = 0;
            attributeDescs[1].location = 1;
            attributeDescs[1].format = VK_FORMAT_R32G32B32A32_SFLOAT;
            attributeDescs[1].offset = offsetof(VulkanLineVertex, Color);

            attributeDescs[2].binding = 0;
            attributeDescs[2].location = 2;
            attributeDescs[2].format = VK_FORMAT_R32_SINT;
            attributeDescs[2].offset = offsetof(VulkanLineVertex, EntityID);

            return attributeDescs;
        }
        */
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
		VkPipeline GetLinePipeline() const { return m_linePipeline; }
        VkPipelineLayout GetGamePipelineLayout() const { return m_gamePipelineLayout; }
        VkPipelineLayout GetPresentPipelineLayout() const { return m_presentPipelineLayout; }
		VkPipelineLayout GetLinePipelineLayout() const { return m_linePipelineLayout; }
        VkDescriptorSet GetGameDescriptorSet(size_t frameIndex) { return m_gameDescriptorSets[frameIndex]; }
        VkDescriptorSet GetCameraDescriptorSet(size_t frameIndex) { return m_cameraDescriptorSets[frameIndex]; }
        VkDescriptorSet GetPresentDescriptorSet(size_t frameIndex) { return m_presentDescriptorSets[frameIndex]; }
		VkDescriptorSet GetLineDescriptorSet() { return m_lineDescriptorSet; }

        VkSampler& GetPresentSampler() { return m_presentSampler; }

    private:

        void CreateGameGraphicsPipeline(VkRenderPass renderPass);
        void CreateLineGraphicsPipeline(VkRenderPass renderPass);
        void CreatePresentGraphicsPipeline(VkRenderPass renderPass);
        void CreatePresentPipelineLayout();
        void CreateDescriptorSetLayouts();
        void CreatePresentGameDescriptorPool();
        void CreateGameAndPresentDescriptorSets();
        void CreateGameDescriptorSet();
        void CreateLineDescriptorSet();
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
        VkPipeline m_linePipeline;
        VkPipelineLayout m_gamePipelineLayout;
        VkPipelineLayout m_linePipelineLayout;
        VkPipelineLayout m_imguiPipelineLayout;
        VkPipelineLayout m_presentPipelineLayout;

        VkDescriptorSetLayout m_gameDescriptorSetLayout;
        VkDescriptorSetLayout m_presentDescriptorSetLayout;
        VkDescriptorSetLayout m_lineDescriptorSetLayout;
        VkDescriptorSetLayout m_cameraDescriptorSetLayout;
        VkDescriptorPool m_presentGamedescriptorPool;
        VkDescriptorSet m_gameDescriptorSet;
        VkDescriptorSet m_presentDescriptorSet;
        VkDescriptorSet m_lineDescriptorSet;

        std::vector<VkDescriptorSet> m_gameDescriptorSets;
        std::vector<VkDescriptorSet> m_cameraDescriptorSets;
        std::vector<VkDescriptorSet> m_presentDescriptorSets;
        VkDescriptorPool m_descriptorPool;
        std::vector<VulkanBuffer> m_uniformBuffers;

        Ref<VulkanShader> m_pixelGameShader;
        Ref<VulkanShader> m_fullscreenShader;
        Ref<VulkanShader> m_lineShader;
        Ref<VulkanShader> m_vulkanRenderShader;
        VkSampler m_presentSampler;

        std::vector<VkDynamicState> m_dynamicStates =
        {
            VK_DYNAMIC_STATE_VIEWPORT,
            VK_DYNAMIC_STATE_SCISSOR
        };

    };

}


