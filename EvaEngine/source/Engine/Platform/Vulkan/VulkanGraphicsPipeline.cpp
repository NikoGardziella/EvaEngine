#include "pch.h"
#include "VulkanGraphicsPipeline.h"
#include "Engine/AssetManager/AssetManager.h"
#include "VulkanBuffer.h"

#include <fstream>
#include <stdexcept>
#include <vector>
#include <Engine/Renderer/Shader.h>
#include "VulkanContext.h"
#include <Engine/Renderer/VulkanRenderer2D.h>
#include <Engine/Renderer/Renderer.h>
#include <backends/imgui_impl_vulkan.h>
#include <backends/imgui_impl_glfw.h>
#include <Engine/Core/Application.h>

namespace Engine {
   // const int MAX_FRAMES_IN_FLIGHT = 3;

  

    VulkanGraphicsPipeline::VulkanGraphicsPipeline(VkDevice device, VkExtent2D swapchainExtent, VkRenderPass renderPass, Ref<VulkanShader> shader)
        : m_device(device),
        m_graphicsPipeline(VK_NULL_HANDLE),
        m_pipelineLayout(VK_NULL_HANDLE)
    {
        

        VulkanContext* vulkanContext = VulkanContext::Get();
        m_descriptorPool = vulkanContext->GetDescriptorPool();
        m_texture = std::make_shared<VulkanTexture>(AssetManager::GetAssetPath("textures/ee_logo.png").string());

        
        m_uniformBuffer = VulkanBuffer(
            m_device,
            vulkanContext->GetDeviceManager().GetPhysicalDevice(),
            sizeof(UniformBufferObject),
            VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
        );
        m_circleShader = std::make_shared<VulkanShader>(AssetManager::GetAssetPath("shaders/VulkanRenderer2D_Quad.GLSL").string());
        //m_circleShader = std::make_shared<VulkanShader>(AssetManager::GetAssetPath("shaders/PixelGameShader.GLSL").string());
       // m_circleShader = shader;
        CreateDescriptorSetLayout();
        CreateDescriptorSet();
        CreateGraphicsPipeline(swapchainExtent, renderPass);


    }

    VulkanGraphicsPipeline::~VulkanGraphicsPipeline()
    {
        vkDestroyPipeline(m_device, m_graphicsPipeline, nullptr);
        vkDestroyPipelineLayout(m_device, m_pipelineLayout, nullptr);

    }

   

   

    void VulkanGraphicsPipeline::CreateGraphicsPipeline(VkExtent2D swapchainExtent, VkRenderPass renderPass)
    {
		
        VkPipelineShaderStageCreateInfo vertShaderStageInfo{};
        vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
        vertShaderStageInfo.module = m_circleShader->GetVertexShaderModule();
        vertShaderStageInfo.pName = "main";

        VkPipelineShaderStageCreateInfo fragShaderStageInfo{};
        fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
        fragShaderStageInfo.module = m_circleShader->GetFragmentShaderModule();
        fragShaderStageInfo.pName = "main";

        VkPipelineShaderStageCreateInfo shaderStages[] = { vertShaderStageInfo, fragShaderStageInfo };

        VkPipelineDynamicStateCreateInfo dynamicState{};
        dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
        dynamicState.dynamicStateCount = static_cast<uint32_t>(m_dynamicStates.size());
        dynamicState.pDynamicStates = m_dynamicStates.data();

        // Define the vertex input binding description
        VkVertexInputBindingDescription bindingDescription{};
        bindingDescription.binding = 0;
        bindingDescription.stride = sizeof(VulkanQuadVertex);
        bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

        // Define the vertex input attribute descriptions
        std::array<VkVertexInputAttributeDescription, 5> attributeDescriptions{};
        attributeDescriptions[0].binding = 0;
        attributeDescriptions[0].location = 0;
        attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
        attributeDescriptions[0].offset = offsetof(VulkanQuadVertex, Position);

        attributeDescriptions[1].binding = 0;
        attributeDescriptions[1].location = 1;
        attributeDescriptions[1].format = VK_FORMAT_R32G32B32A32_SFLOAT;
        attributeDescriptions[1].offset = offsetof(VulkanQuadVertex, Color);

        attributeDescriptions[2].binding = 0;
        attributeDescriptions[2].location = 2;
        attributeDescriptions[2].format = VK_FORMAT_R32G32_SFLOAT;
        attributeDescriptions[2].offset = offsetof(VulkanQuadVertex, TexCoord);

        attributeDescriptions[3].binding = 0;
        attributeDescriptions[3].location = 3;
        attributeDescriptions[3].format = VK_FORMAT_R32_SFLOAT;
        attributeDescriptions[3].offset = offsetof(VulkanQuadVertex, TexIndex);

        attributeDescriptions[4].binding = 0;
        attributeDescriptions[4].location = 4;
        attributeDescriptions[4].format = VK_FORMAT_R32_SFLOAT;
        attributeDescriptions[4].offset = offsetof(VulkanQuadVertex, TilingFactor);


        //format of the vertex data that will be passed to the vertex shader.
        VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
        vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
        vertexInputInfo.vertexBindingDescriptionCount = 1;
        vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;
        vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size());
        vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data();

        // what kind of geometry will be drawn from the vertices and if primitive restart should be enabled
        VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
        inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
        inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        inputAssembly.primitiveRestartEnable = VK_FALSE;

        VkViewport viewport{};
        viewport.x = 0.0f;
        viewport.y = 0.0f;
        viewport.width = static_cast<float>(swapchainExtent.width);
        viewport.height = static_cast<float>(swapchainExtent.height);
        viewport.minDepth = 0.0f;
        viewport.maxDepth = 1.0f;

        VkRect2D scissor{};
        scissor.offset = { 0, 0 };
        scissor.extent = swapchainExtent;

        VkPipelineViewportStateCreateInfo viewportState{};
        viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
        viewportState.viewportCount = 1;
        viewportState.pViewports = &viewport;
        viewportState.scissorCount = 1;
        viewportState.pScissors = &scissor;

        // takes the geometry that is shaped by the vertices from the vertex
        // shader and turns it into fragments to be colored by the fragment shader. 
        VkPipelineRasterizationStateCreateInfo rasterizer{};
        rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
        rasterizer.depthClampEnable = VK_FALSE;
        rasterizer.rasterizerDiscardEnable = VK_FALSE;
        rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
        rasterizer.lineWidth = 1.0f;
        rasterizer.cullMode = VK_CULL_MODE_NONE;
        rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE;
        rasterizer.depthBiasEnable = VK_FALSE;
        rasterizer.depthBiasConstantFactor = 0.0f; // Optional
        rasterizer.depthBiasClamp = 0.0f; // Optional
        rasterizer.depthBiasSlopeFactor = 0.0f; // Optional


        VkPipelineMultisampleStateCreateInfo multisampling{};
        multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
        multisampling.sampleShadingEnable = VK_FALSE;
        multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
        multisampling.minSampleShading = 1.0f; // Optional
        multisampling.pSampleMask = nullptr; // Optional
        multisampling.alphaToCoverageEnable = VK_FALSE; // Optional
        multisampling.alphaToOneEnable = VK_FALSE; // Optional


        VkPipelineColorBlendAttachmentState colorBlendAttachment{};
        colorBlendAttachment.blendEnable = VK_TRUE;  // Enable blending
        colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA; // Use alpha for source blend
        colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA; // Use inverse alpha for destination
        colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
        colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA; // Use alpha for source alpha
        colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA; // Use inverse alpha for destination alpha
        colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;
        colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
            VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;

        VkPipelineColorBlendStateCreateInfo colorBlending{};
        colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
        colorBlending.logicOpEnable = VK_FALSE;
        colorBlending.logicOp = VK_LOGIC_OP_COPY; // Optional
        colorBlending.attachmentCount = 1;
        colorBlending.pAttachments = &colorBlendAttachment;
        colorBlending.blendConstants[0] = 0.0f;  // Optional
        colorBlending.blendConstants[1] = 0.0f;  // Optional
        colorBlending.blendConstants[2] = 0.0f;  // Optional
        colorBlending.blendConstants[3] = 0.0f;  // Optional

        
        VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
        pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        pipelineLayoutInfo.pushConstantRangeCount = 0; // Optional
        pipelineLayoutInfo.pPushConstantRanges = nullptr; // Optional
        pipelineLayoutInfo.setLayoutCount = 1; // Ensure this is NOT zero
        pipelineLayoutInfo.pSetLayouts = &m_descriptorSetLayout;


        if (vkCreatePipelineLayout(m_device, &pipelineLayoutInfo, nullptr, &m_pipelineLayout) != VK_SUCCESS)
        {
			EE_CORE_ASSERT(false, "failed to create pipeline layout!");
        }
        else
        {
			EE_CORE_INFO("Vulkan pipeline layout created");
        }
   
        VkGraphicsPipelineCreateInfo pipelineInfo{};
        pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;

        pipelineInfo.stageCount = 2;
        pipelineInfo.pStages = shaderStages;
        pipelineInfo.pVertexInputState = &vertexInputInfo;
        pipelineInfo.pInputAssemblyState = &inputAssembly;
        pipelineInfo.pViewportState = &viewportState;
        pipelineInfo.pRasterizationState = &rasterizer;
        pipelineInfo.pMultisampleState = &multisampling;
        pipelineInfo.pDepthStencilState = nullptr; // Optional
        pipelineInfo.pColorBlendState = &colorBlending;
        pipelineInfo.pDynamicState = &dynamicState;
        pipelineInfo.layout = m_pipelineLayout;
        pipelineInfo.renderPass = renderPass;
        pipelineInfo.subpass = 0;
        pipelineInfo.basePipelineHandle = VK_NULL_HANDLE; // Optional
        pipelineInfo.basePipelineIndex = -1; // Optional

       
        if (vkCreateGraphicsPipelines(m_device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &m_graphicsPipeline) != VK_SUCCESS)
        {
			EE_CORE_ASSERT(false, "failed to create graphics pipeline!");
        }
        else
        {
			EE_CORE_INFO("Vulkan graphics pipeline created");
        }

    }

    void VulkanGraphicsPipeline::CreateDescriptorSetLayout()
    {
        VkDescriptorSetLayoutBinding uboBinding{};
        uboBinding.binding = 0;
        uboBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        uboBinding.descriptorCount = 1;
        uboBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
        uboBinding.pImmutableSamplers = nullptr;

        VkDescriptorSetLayoutBinding samplerBinding{};
        samplerBinding.binding = 1;
        samplerBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        samplerBinding.descriptorCount = 32; // number of textures
        samplerBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
        samplerBinding.pImmutableSamplers = nullptr;

        std::array<VkDescriptorSetLayoutBinding, 2> bindings = { uboBinding, samplerBinding };

        VkDescriptorSetLayoutCreateInfo layoutInfo{};
        layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
        layoutInfo.pBindings = bindings.data();

        if (vkCreateDescriptorSetLayout(m_device, &layoutInfo, nullptr, &m_descriptorSetLayout) != VK_SUCCESS)
        {
            EE_CORE_ASSERT(false, "failed to create descriptor set layout!");
        }
        else
        {
            EE_CORE_INFO("Vulkan descriptor set layout created");
        }

    
    }

    void VulkanGraphicsPipeline::CreateDescriptorSet()
    {
        m_descriptorSets.resize(MAX_FRAMES_IN_FLIGHT);

        std::vector<VkDescriptorSetLayout> layouts(MAX_FRAMES_IN_FLIGHT, m_descriptorSetLayout);
        VkDescriptorSetAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        allocInfo.descriptorPool = m_descriptorPool;
        allocInfo.descriptorSetCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
        allocInfo.pSetLayouts = layouts.data();

        if (vkAllocateDescriptorSets(m_device, &allocInfo, m_descriptorSets.data()) != VK_SUCCESS)
        {
			EE_CORE_ASSERT(false, "failed to allocate descriptor sets!");
        }
        else
        {
			EE_CORE_INFO("Vulkan descriptor sets allocated");
        }

        for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
        {
            UpdateDescriptorSets(i);
        }
    }



    void VulkanGraphicsPipeline::UpdateDescriptorSets(size_t frameIndex)
    {
       

        VkDescriptorBufferInfo bufferInfo{};
        bufferInfo.buffer = m_uniformBuffer.GetBuffer();
        bufferInfo.offset = 0;
        bufferInfo.range = sizeof(glm::mat4);

        VkWriteDescriptorSet uboWrite{};
        uboWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        uboWrite.dstSet = m_descriptorSets[frameIndex];
        uboWrite.dstBinding = 0;
        uboWrite.dstArrayElement = 0;
        uboWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        uboWrite.descriptorCount = 1;
        uboWrite.pBufferInfo = &bufferInfo;

        std::array<VkDescriptorImageInfo, 32> imageInfos{};
        for (size_t i = 0; i < imageInfos.size(); i++)
        {
            imageInfos[i].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            imageInfos[i].imageView = m_texture->GetImageView();
            imageInfos[i].sampler = m_texture->GetSampler();
        }

        VkWriteDescriptorSet samplerWrite{};
        samplerWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        samplerWrite.dstSet = m_descriptorSets[frameIndex];
        samplerWrite.dstBinding = 1;
        samplerWrite.dstArrayElement = 0;
        samplerWrite.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        samplerWrite.descriptorCount = static_cast<uint32_t>(imageInfos.size());
        samplerWrite.pImageInfo = imageInfos.data();

        std::array<VkWriteDescriptorSet, 2> descriptorWrites = { uboWrite, samplerWrite };
        vkUpdateDescriptorSets(m_device, static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
    }

    void VulkanGraphicsPipeline::UpdateDescriptorSets(uint32_t slotIndex, const Ref<VulkanTexture>& texture)
    {
        EE_CORE_ASSERT(slotIndex < 32, "Texture slot index out of bounds!");

        VkDescriptorImageInfo imageInfo{};
        imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        imageInfo.imageView = texture->GetImageView();
        imageInfo.sampler = texture->GetSampler();

        for (size_t i = 0; i < m_descriptorSets.size(); ++i)
        {
            VkWriteDescriptorSet samplerWrite{};
            samplerWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            samplerWrite.dstSet = m_descriptorSets[i];
            samplerWrite.dstBinding = 1; // assuming all textures are at binding = 1
            samplerWrite.dstArrayElement = slotIndex;
            samplerWrite.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
            samplerWrite.descriptorCount = 1;
            samplerWrite.pImageInfo = &imageInfo;

            vkUpdateDescriptorSets(m_device, 1, &samplerWrite, 0, nullptr);
        }
    }


    void VulkanGraphicsPipeline::UpdateUniformBuffer(const glm::mat4& viewProjectionMatrix)
    {
        void* data;
        vkMapMemory(m_device, m_uniformBuffer.GetMemory(), 0, sizeof(viewProjectionMatrix), 0, &data);
        memcpy(data, &viewProjectionMatrix, sizeof(viewProjectionMatrix));
        vkUnmapMemory(m_device, m_uniformBuffer.GetMemory());
    }

  

}
