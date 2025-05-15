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


    VulkanGraphicsPipeline::VulkanGraphicsPipeline(VulkanContext& vulkanContext)

    {
		m_swapchainExtent = vulkanContext.GetVulkanSwapchain().GetSwapchainExtent();
	
		m_device = vulkanContext.GetDeviceManager().GetDevice();
        m_descriptorPool = vulkanContext.GetDescriptorPool();

        m_pixelGameShader = std::make_shared<VulkanShader>(AssetManager::GetAssetPath("shaders/PixelGameShader.GLSL").string());
        m_fullscreenShader = std::make_shared<VulkanShader>(AssetManager::GetAssetPath("shaders/fullscreen_shader.GLSL").string());
        m_lineShader = std::make_shared<VulkanShader>(AssetManager::GetAssetPath("shaders/Line_shader.GLSL").string());

        m_uniformBuffers.resize(MAX_FRAMES_IN_FLIGHT);
        for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
        {
            m_uniformBuffers[i] = VulkanBuffer(
                m_device,
                vulkanContext.GetDeviceManager().GetPhysicalDevice(),
                sizeof(glm::mat4),
                VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
            );
        }
        m_vulkanRenderShader = std::make_shared<VulkanShader>(AssetManager::GetAssetPath("shaders/VulkanRenderer2D_Quad.GLSL").string());
       
        CreatePresentSampler();

        CreateDescriptorSetLayout();
        CreateDescriptorSetLayouts();
        CreateCameraDescriptorSetLayout();

        CreateLineGraphicsPipeline(vulkanContext.GetGameRenderPass());

        CreateGameGraphicsPipeline(vulkanContext.GetGameRenderPass());
        CreateGameDescriptorSet();
        CreateCameraDescriptorSet();
        CreatePresentDescriptorSet();
        CreateLineDescriptorSet();

        CreatePresentGameDescriptorPool();
        CreateGameAndPresentDescriptorSets();

        CreatePresentPipelineLayout();
        CreatePresentGraphicsPipeline(vulkanContext.GetPresentRenderPass());

    }

    VulkanGraphicsPipeline::~VulkanGraphicsPipeline()
    {
        vkDestroyPipeline(m_device, m_gameGraphicsPipeline, nullptr);
        vkDestroyPipelineLayout(m_device, m_gamePipelineLayout, nullptr);
        vkDestroyPipelineLayout(m_device, m_imguiPipelineLayout, nullptr);
        vkDestroyPipelineLayout(m_device, m_linePipelineLayout, nullptr);
    }

 
    void VulkanGraphicsPipeline::CreateGameGraphicsPipeline(VkRenderPass renderPass)
    {	
        VkPipelineShaderStageCreateInfo vertShaderStageInfo{};
        vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
        vertShaderStageInfo.module = m_vulkanRenderShader->GetVertexShaderModule();
        vertShaderStageInfo.pName = "main";

        VkPipelineShaderStageCreateInfo fragShaderStageInfo{};
        fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
        fragShaderStageInfo.module = m_vulkanRenderShader->GetFragmentShaderModule();
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
        viewport.width = static_cast<float>(m_swapchainExtent.width);
        viewport.height = static_cast<float>(m_swapchainExtent.height);
        viewport.minDepth = 0.0f;
        viewport.maxDepth = 1.0f;

        VkRect2D scissor{};
        scissor.offset = { 0, 0 };
        scissor.extent = m_swapchainExtent;

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
        colorBlendAttachment.blendEnable = VK_TRUE;  // for alpha channel
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

        
        VkDescriptorSetLayout setLayouts[] = {
            m_cameraDescriptorSetLayout,  
            m_gameDescriptorSetLayout   
        };

        VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
        pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        pipelineLayoutInfo.pushConstantRangeCount = 0; // Optional
        pipelineLayoutInfo.pPushConstantRanges = nullptr; // Optional
        pipelineLayoutInfo.setLayoutCount = 2; // Ensure this is NOT zero
        pipelineLayoutInfo.pSetLayouts = setLayouts;


        if (vkCreatePipelineLayout(m_device, &pipelineLayoutInfo, nullptr, &m_gamePipelineLayout) != VK_SUCCESS)
        {
			EE_CORE_ASSERT(false, "failed to create pipeline layout!");
        }
        else
        {
			EE_CORE_INFO("Vulkan game pipeline layout created");
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
        pipelineInfo.layout = m_gamePipelineLayout;
        pipelineInfo.renderPass = renderPass;
        pipelineInfo.subpass = 0;
        pipelineInfo.basePipelineHandle = VK_NULL_HANDLE; // Optional
        pipelineInfo.basePipelineIndex = -1; // Optional

       
        if (vkCreateGraphicsPipelines(m_device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &m_gameGraphicsPipeline) != VK_SUCCESS)
        {
			EE_CORE_ASSERT(false, "failed to create game  graphics pipeline!");
        }
        else
        {
			EE_CORE_INFO("Vulkan game  graphics pipeline created");
        }

    }

    void VulkanGraphicsPipeline::CreateLineGraphicsPipeline(VkRenderPass renderPass)
    {

        // Shader Stages
        VkPipelineShaderStageCreateInfo vertStage{};
        vertStage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        vertStage.stage = VK_SHADER_STAGE_VERTEX_BIT;
        vertStage.module = m_lineShader->GetVertexShaderModule();
        vertStage.pName = "main";

        VkPipelineShaderStageCreateInfo fragStage{};
        fragStage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        fragStage.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
        fragStage.module = m_lineShader->GetFragmentShaderModule();
        fragStage.pName = "main";

        VkPipelineShaderStageCreateInfo shaderStages[] = { vertStage, fragStage };

        VkVertexInputBindingDescription bindingDescription{};
        bindingDescription.binding = 0;
        bindingDescription.stride = sizeof(VulkanLineVertex);
        bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

        std::array<VkVertexInputAttributeDescription, 2> attributeDescriptions{};

        attributeDescriptions[0].binding = 0;
        attributeDescriptions[0].location = 0;
        attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
        attributeDescriptions[0].offset = offsetof(VulkanLineVertex, Position);

        attributeDescriptions[1].binding = 0;
        attributeDescriptions[1].location = 1;
        attributeDescriptions[1].format = VK_FORMAT_R32G32B32A32_SFLOAT;
        attributeDescriptions[1].offset = offsetof(VulkanLineVertex, Color);

        

        // Vertex Input (none for fullscreen triangle)
        VkPipelineVertexInputStateCreateInfo vertexInput{};
        vertexInput.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
        vertexInput.pNext = nullptr;
        vertexInput.flags = 0;
        vertexInput.vertexBindingDescriptionCount = 1;
        vertexInput.pVertexBindingDescriptions = &bindingDescription;
        vertexInput.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size());
        vertexInput.pVertexAttributeDescriptions = attributeDescriptions.data();


        VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
        inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
        inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_LINE_LIST; // Important for lines
        inputAssembly.primitiveRestartEnable = VK_FALSE;

        VkPipelineRasterizationStateCreateInfo rasterizer{};
        rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
        rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
        rasterizer.lineWidth = 1.0f;
        rasterizer.cullMode = VK_CULL_MODE_NONE;
        rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE;

        // Viewport and Scissor (dynamic preferred)
        VkPipelineViewportStateCreateInfo viewportState{};
        viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
        viewportState.viewportCount = 1;
        viewportState.scissorCount = 1;


        // Multisampling
        VkPipelineMultisampleStateCreateInfo multisampling{};
        multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
        multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

        // Color Blending
        VkPipelineColorBlendAttachmentState colorBlendAttachment{};
        colorBlendAttachment.blendEnable = VK_FALSE;
        colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
        colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
        colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT |
            VK_COLOR_COMPONENT_G_BIT |
            VK_COLOR_COMPONENT_B_BIT |
            VK_COLOR_COMPONENT_A_BIT;

        VkPipelineColorBlendStateCreateInfo colorBlending{};
        colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
        colorBlending.attachmentCount = 1;
        colorBlending.pAttachments = &colorBlendAttachment;

        std::vector<VkDynamicState> dynamicStates = {
            VK_DYNAMIC_STATE_VIEWPORT,
            VK_DYNAMIC_STATE_SCISSOR,
            VK_DYNAMIC_STATE_LINE_WIDTH,
        };
        VkPipelineDynamicStateCreateInfo dynamicState{};
        dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
        dynamicState.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
        dynamicState.pDynamicStates = dynamicStates.data();

        // Pipeline Layout (with your descriptor set layout)
        VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
        pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        pipelineLayoutInfo.setLayoutCount = 1;
        pipelineLayoutInfo.pSetLayouts = &m_lineDescriptorSetLayout;


        VkResult result = vkCreatePipelineLayout(m_device, &pipelineLayoutInfo, nullptr, &m_linePipelineLayout);
        if (result != VK_SUCCESS)
        {
            EE_CORE_ASSERT(false, "failed to create line  pipeline layout!");
        }

        // Graphics Pipeline
        VkGraphicsPipelineCreateInfo pipelineInfo{};
        pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
        pipelineInfo.stageCount = 2;
        pipelineInfo.pStages = shaderStages;
        pipelineInfo.pVertexInputState = &vertexInput;
        pipelineInfo.pInputAssemblyState = &inputAssembly;
        pipelineInfo.pViewportState = &viewportState;
        pipelineInfo.pRasterizationState = &rasterizer;
        pipelineInfo.pMultisampleState = &multisampling;
        pipelineInfo.pColorBlendState = &colorBlending;
        pipelineInfo.pDynamicState = &dynamicState;
        pipelineInfo.layout = m_linePipelineLayout;

        pipelineInfo.renderPass = renderPass;  // render pass for swapchain
        pipelineInfo.subpass = 0;

       
        if (vkCreateGraphicsPipelines(m_device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &m_linePipeline) != VK_SUCCESS)
        {
            EE_CORE_ASSERT(false, "failed to create line graphics pipeline!");
        }
        else
        {
            EE_CORE_INFO("Vulkan line graphics pipeline created");
        }


    }

    void VulkanGraphicsPipeline::CreatePresentGraphicsPipeline(VkRenderPass renderPass)
    {
        // Load SPIR-V

        // Shader Stages
        VkPipelineShaderStageCreateInfo vertStage{};
        vertStage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        vertStage.stage = VK_SHADER_STAGE_VERTEX_BIT;
        vertStage.module = m_fullscreenShader->GetVertexShaderModule();
        vertStage.pName = "main";

        VkPipelineShaderStageCreateInfo fragStage{};
        fragStage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        fragStage.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
        fragStage.module = m_fullscreenShader->GetFragmentShaderModule();
        fragStage.pName = "main";

        VkPipelineShaderStageCreateInfo shaderStages[] = { vertStage, fragStage };

        // Vertex Input (none for fullscreen triangle)
        VkPipelineVertexInputStateCreateInfo vertexInput{};
        vertexInput.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
        vertexInput.vertexBindingDescriptionCount = 0;
        vertexInput.pVertexBindingDescriptions = nullptr;
        vertexInput.vertexAttributeDescriptionCount = 0;
        vertexInput.pVertexAttributeDescriptions = nullptr;

        // Input Assembly
        VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
        inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
        inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

        // Viewport and Scissor (dynamic preferred)
        VkPipelineViewportStateCreateInfo viewportState{};
        viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
        viewportState.viewportCount = 1;
        viewportState.scissorCount = 1;

        // Rasterizer
        VkPipelineRasterizationStateCreateInfo rasterizer{};
        rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
        rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
        rasterizer.lineWidth = 1.0f;
        rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
        rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE;

        // Multisampling
        VkPipelineMultisampleStateCreateInfo multisampling{};
        multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
        multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

        // Color Blending
        VkPipelineColorBlendAttachmentState colorBlendAttachment{};
        colorBlendAttachment.blendEnable = VK_TRUE;
        colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
        colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
        colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT |
            VK_COLOR_COMPONENT_G_BIT |
            VK_COLOR_COMPONENT_B_BIT |
            VK_COLOR_COMPONENT_A_BIT;

        VkPipelineColorBlendStateCreateInfo colorBlending{};
        colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
        colorBlending.attachmentCount = 1;
        colorBlending.pAttachments = &colorBlendAttachment;

        // Dynamic State (optional)
        std::vector<VkDynamicState> dynamicStates = {
            VK_DYNAMIC_STATE_VIEWPORT,
            VK_DYNAMIC_STATE_SCISSOR,
        };
        VkPipelineDynamicStateCreateInfo dynamicState{};
        dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
        dynamicState.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
        dynamicState.pDynamicStates = dynamicStates.data();

        // Pipeline Layout (with your descriptor set layout)
        VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
        pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        pipelineLayoutInfo.setLayoutCount = 1;
        pipelineLayoutInfo.pSetLayouts = &m_presentDescriptorSetLayout;

        vkCreatePipelineLayout(m_device, &pipelineLayoutInfo, nullptr, &m_presentPipelineLayout);

        // Graphics Pipeline
        VkGraphicsPipelineCreateInfo pipelineInfo{};
        pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
        pipelineInfo.stageCount = 2;
        pipelineInfo.pStages = shaderStages;
        pipelineInfo.pVertexInputState = &vertexInput;
        pipelineInfo.pInputAssemblyState = &inputAssembly;
        pipelineInfo.pViewportState = &viewportState;
        pipelineInfo.pRasterizationState = &rasterizer;
        pipelineInfo.pMultisampleState = &multisampling;
        pipelineInfo.pColorBlendState = &colorBlending;
        pipelineInfo.pDynamicState = &dynamicState;
        pipelineInfo.layout = m_presentPipelineLayout;
        pipelineInfo.renderPass = renderPass;  // render pass for swapchain
        pipelineInfo.subpass = 0;

        
        if (vkCreateGraphicsPipelines(m_device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &m_presentPipeline) != VK_SUCCESS)
        {
            EE_CORE_ASSERT(false, "failed to create present graphics pipeline!");
        }
        else
        {
            EE_CORE_INFO("Vulkan present graphics pipeline created");
        }

    }

 
    void VulkanGraphicsPipeline::CreatePresentPipelineLayout()
    {
        VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
        pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        pipelineLayoutInfo.setLayoutCount = 1;
        pipelineLayoutInfo.pSetLayouts = &m_presentDescriptorSetLayout;

        vkCreatePipelineLayout(m_device, &pipelineLayoutInfo, nullptr, &m_presentPipelineLayout);

    }

    void VulkanGraphicsPipeline::CreateDescriptorSetLayouts()
    {
        // Two bindings: Camera UBO and Texture Sampler Array
        VkDescriptorSetLayoutBinding bindings[2] = {};

        // Binding 0 - Camera Uniform Buffer
        bindings[0].binding = 0;
        bindings[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        bindings[0].descriptorCount = 1;
        bindings[0].stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
        bindings[0].pImmutableSamplers = nullptr;

        // Binding 1 - Array of 32 combined image samplers
        bindings[1].binding = 1;
        bindings[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        bindings[1].descriptorCount = 32; // 32 textures
        bindings[1].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
        bindings[1].pImmutableSamplers = nullptr;

        VkDescriptorSetLayoutCreateInfo layoutInfo = {};
        layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        layoutInfo.bindingCount = 2;
        layoutInfo.pBindings = bindings;

        if (vkCreateDescriptorSetLayout(m_device, &layoutInfo, nullptr, &m_gameDescriptorSetLayout) != VK_SUCCESS)
        {
            EE_CORE_ASSERT(false, "failed to create DescriptorSet Layout!");

        }

        VkDescriptorSetLayoutBinding presentBindings[1] = {};
        presentBindings[0].binding = 0;
        presentBindings[0].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        presentBindings[0].descriptorCount = 1;
        presentBindings[0].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
        presentBindings[0].pImmutableSamplers = nullptr;

        VkDescriptorSetLayoutCreateInfo presentLayoutInfo = {};
        presentLayoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        presentLayoutInfo.bindingCount = 1;
        presentLayoutInfo.pBindings = presentBindings;

        VkResult result = vkCreateDescriptorSetLayout(m_device, &presentLayoutInfo, nullptr, &m_presentDescriptorSetLayout);
        if (result != VK_SUCCESS)
        {
            EE_CORE_ASSERT(false, "Failed to create present descriptor set layout!");
        }

        VkDescriptorSetLayoutBinding lineLayoutBinding{};
        lineLayoutBinding.binding = 0;
        lineLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        lineLayoutBinding.descriptorCount = 1;
        lineLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT; // Used only in vertex shader
        lineLayoutBinding.pImmutableSamplers = nullptr;

        VkDescriptorSetLayoutCreateInfo lineLayoutInfo{};
        lineLayoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        lineLayoutInfo.bindingCount = 1;
        lineLayoutInfo.pBindings = &lineLayoutBinding;

        result = vkCreateDescriptorSetLayout(m_device, &lineLayoutInfo, nullptr, &m_lineDescriptorSetLayout);
        if (result != VK_SUCCESS)
        {
            EE_CORE_ASSERT(false, "Failed to create line descriptor set layout!");
        }


    }

    void VulkanGraphicsPipeline::CreatePresentGameDescriptorPool()
    {
        VkDescriptorPoolSize poolSizes[1] = {};
        poolSizes[0].type = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
        poolSizes[0].descriptorCount = 2;  // One for rendering and one for presentation

        VkDescriptorPoolCreateInfo poolCreateInfo = {};
        poolCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        poolCreateInfo.poolSizeCount = 1;
        poolCreateInfo.pPoolSizes = poolSizes;
        poolCreateInfo.maxSets = 2;  // One for rendering and one for presentation

        vkCreateDescriptorPool(m_device, &poolCreateInfo, nullptr, &m_presentGamedescriptorPool);
    }

    void VulkanGraphicsPipeline::CreateGameAndPresentDescriptorSets()
    {
        // Allocate descriptor sets for rendering and presentation
        VkDescriptorSetAllocateInfo allocateInfo = {};
        allocateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        allocateInfo.descriptorPool = m_presentGamedescriptorPool;

        // Create descriptor set for game rendering
        allocateInfo.descriptorSetCount = 1;  // Set the count to 1
        allocateInfo.pSetLayouts = &m_gameDescriptorSetLayout;
        VkResult result = vkAllocateDescriptorSets(m_device, &allocateInfo, &m_gameDescriptorSet);
        if (result != VK_SUCCESS)
        {
            throw std::runtime_error("Failed to allocate game descriptor set.");
        }

        // Create descriptor set for presentation
        allocateInfo.pSetLayouts = &m_presentDescriptorSetLayout;
        result = vkAllocateDescriptorSets(m_device, &allocateInfo, &m_presentDescriptorSet);
        if (result != VK_SUCCESS)
        {
            throw std::runtime_error("Failed to allocate presentation descriptor set.");
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

        if (vkCreateDescriptorSetLayout(m_device, &layoutInfo, nullptr, &m_gameDescriptorSetLayout) != VK_SUCCESS)
        {
            EE_CORE_ASSERT(false, "failed to create descriptor set layout!");
        }
        else
        {
            EE_CORE_INFO("Vulkan descriptor set layout created");
        }

    
    }

    void VulkanGraphicsPipeline::CreateGameDescriptorSet()
    {
        m_gameDescriptorSets.resize(MAX_FRAMES_IN_FLIGHT);

        std::vector<VkDescriptorSetLayout> layouts(MAX_FRAMES_IN_FLIGHT, m_gameDescriptorSetLayout);
        VkDescriptorSetAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        allocInfo.descriptorPool = m_descriptorPool;
        allocInfo.descriptorSetCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
        allocInfo.pSetLayouts = layouts.data();


        if (vkAllocateDescriptorSets(m_device, &allocInfo, m_gameDescriptorSets.data()) != VK_SUCCESS)
        {
			EE_CORE_ASSERT(false, "failed to allocate descriptor sets!");
        }
        else
        {
			EE_CORE_INFO("Vulkan descriptor sets allocated");
        }

        for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
        {
           // UpdateGameDescriptorSets(i);
        }
        //UpdateCameraUBODescriptorSets();
    }
    void VulkanGraphicsPipeline::CreateLineDescriptorSet()
    {
        VkDescriptorSetAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        allocInfo.descriptorPool = m_descriptorPool; // your existing pool
        allocInfo.descriptorSetCount = 1;
        allocInfo.pSetLayouts = &m_lineDescriptorSetLayout;

        if (vkAllocateDescriptorSets(m_device, &allocInfo, &m_lineDescriptorSet) != VK_SUCCESS)
        {
            throw std::runtime_error("Failed to allocate line descriptor set!");
        }

        // Update the Descriptor Set with Camera UBO
        VkDescriptorBufferInfo bufferInfo{};
        bufferInfo.buffer = m_uniformBuffers[0].GetBuffer();
        bufferInfo.offset = 0;
        bufferInfo.range = sizeof(glm::mat4);

        VkWriteDescriptorSet descriptorWrite{};
        descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrite.dstSet = m_lineDescriptorSet;
        descriptorWrite.dstBinding = 0;
        descriptorWrite.dstArrayElement = 0;
        descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        descriptorWrite.descriptorCount = 1;
        descriptorWrite.pBufferInfo = &bufferInfo;

        vkUpdateDescriptorSets(m_device, 1, &descriptorWrite, 0, nullptr);

    }

    void VulkanGraphicsPipeline::CreatePresentDescriptorSet()
    {

        m_presentDescriptorSets.resize(MAX_FRAMES_IN_FLIGHT);

        VulkanContext* context = VulkanContext::Get();
        for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
        {
            // Allocate the descriptor set from the descriptor pool (assumed to be pre-created)
            VkDescriptorSetAllocateInfo allocInfo{};
            allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
            allocInfo.descriptorPool = context->GetDescriptorPool(); // Descriptor pool used for allocation
            allocInfo.descriptorSetCount = 1;
            allocInfo.pSetLayouts = &m_presentDescriptorSetLayout; // Layout for the present pass

            // Allocate the descriptor set for the present pass
            VkResult result = vkAllocateDescriptorSets(m_device, &allocInfo, &m_presentDescriptorSets[i]);
            if (result != VK_SUCCESS)
            {
                throw std::runtime_error("Failed to allocate present descriptor set!");
            }

            // Update the present descriptor set with resources (swapchain image, etc.)
            UpdatePresentDescriptorSet(i);

        }
    }

    void VulkanGraphicsPipeline::UpdatePresentDescriptorSet(uint32_t imageIndex)
    {
        VulkanContext* context = VulkanContext::Get();

        // Create a descriptor write for the swapchain image (assuming it's a sampled image)
        VkDescriptorImageInfo imageInfo{};
        imageInfo.sampler = m_presentSampler; // Use the appropriate sampler (could be a default one)
        imageInfo.imageView = context->GetVulkanSwapchain().GetGameTrackedImage(imageIndex).view; // Swapchain image view
        imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL; // Image layout for reading in fragment shader

        VkWriteDescriptorSet writeSet{};
        writeSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        writeSet.dstSet = m_presentDescriptorSets[imageIndex]; // The descriptor set to update
        writeSet.dstBinding = 0; // Binding index for the present pass (adjust accordingly)
        writeSet.dstArrayElement = 0;
        writeSet.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        writeSet.descriptorCount = 1;
        writeSet.pImageInfo = &imageInfo;

        // Update the present descriptor set
        vkUpdateDescriptorSets(m_device, 1, &writeSet, 0, nullptr);
    }

    void VulkanGraphicsPipeline::UpdateTrackedImageDescriptorSets(size_t frameIndex, const std::array<Ref<VulkanTexture>, 32>& textures)
    {
        std::array<VkDescriptorImageInfo, 32> imageInfos{};
        for (uint32_t i = 0; i < 32; ++i)
        {
            imageInfos[i].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            imageInfos[i].imageView = textures[i]->GetImageView();
            imageInfos[i].sampler = textures[i]->GetSampler();
        }

        VkWriteDescriptorSet write{};
        write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        write.dstSet = m_gameDescriptorSets[frameIndex];
        write.dstBinding = 1;
        write.dstArrayElement = 0;
        write.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        write.descriptorCount = static_cast<uint32_t>(imageInfos.size());
        write.pImageInfo = imageInfos.data();

        vkUpdateDescriptorSets(m_device, 1, &write, 0, nullptr);
    }

    void VulkanGraphicsPipeline::CreatePresentSampler()
    {
        VkSamplerCreateInfo samplerInfo{};
        samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
        samplerInfo.magFilter = VK_FILTER_LINEAR;
        samplerInfo.minFilter = VK_FILTER_LINEAR;
        samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        samplerInfo.anisotropyEnable = VK_FALSE;
        samplerInfo.maxAnisotropy = 1.0f;
        samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
        samplerInfo.unnormalizedCoordinates = VK_FALSE;
        samplerInfo.compareEnable = VK_FALSE;
        samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
        samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
        if (vkCreateSampler(m_device, &samplerInfo, nullptr, &m_presentSampler) != VK_SUCCESS)
        {
            EE_CORE_ASSERT(false, "Failed to create offscreen sampler!");
        }
        else
        {
            EE_CORE_INFO("Offscreen sampler created successfully");
        }

    }

	void VulkanGraphicsPipeline::CreateCameraDescriptorSetLayout()
	{

        VkDescriptorSetLayoutBinding cameraBinding{};
        cameraBinding.binding = 0;
        cameraBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        cameraBinding.descriptorCount = 1;
        cameraBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
        cameraBinding.pImmutableSamplers = nullptr;

        VkDescriptorSetLayoutCreateInfo cameraLayoutInfo{};
        cameraLayoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        cameraLayoutInfo.bindingCount = 1;
        cameraLayoutInfo.pBindings = &cameraBinding;

        vkCreateDescriptorSetLayout(m_device, &cameraLayoutInfo, nullptr, &m_cameraDescriptorSetLayout);

	}

    void VulkanGraphicsPipeline::CreateCameraDescriptorSet()
    {
        m_cameraDescriptorSets.resize(MAX_FRAMES_IN_FLIGHT);

        std::vector<VkDescriptorSetLayout> layouts(MAX_FRAMES_IN_FLIGHT, m_cameraDescriptorSetLayout);
        VkDescriptorSetAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        allocInfo.descriptorPool = m_descriptorPool;
        allocInfo.descriptorSetCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
        allocInfo.pSetLayouts = layouts.data();


        if (vkAllocateDescriptorSets(m_device, &allocInfo, m_cameraDescriptorSets.data()) != VK_SUCCESS)
        {
            EE_CORE_ASSERT(false, "failed to allocate descriptor sets!");
        }
        else
        {
            EE_CORE_INFO("Vulkan camera descriptor sets allocated");
        }
       
        UpdateCameraUBODescriptorSets();
    }

    void VulkanGraphicsPipeline::UpdateCameraUBODescriptorSets()
    {
        for (size_t i = 0; i < m_cameraDescriptorSets.size(); ++i)
        {
            VkDescriptorBufferInfo cameraBufferInfo{};
            cameraBufferInfo.buffer = m_uniformBuffers[i].GetBuffer();;
            cameraBufferInfo.offset = 0;
            cameraBufferInfo.range = sizeof(glm::mat4);

            VkWriteDescriptorSet cameraWrite{};
            cameraWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            cameraWrite.dstSet = m_cameraDescriptorSets[i]; // Per-frame descriptor set
            cameraWrite.dstBinding = 0; // Camera UBO is at binding = 0
            cameraWrite.dstArrayElement = 0;
            cameraWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
            cameraWrite.descriptorCount = 1;
            cameraWrite.pBufferInfo = &cameraBufferInfo;

            vkUpdateDescriptorSets(m_device, 1, &cameraWrite, 0, nullptr);
        }

    }

    void VulkanGraphicsPipeline::UpdateUniformBuffer(uint32_t currentFrame, const glm::mat4& viewProjectionMatrix)
    {
        void* data;
        vkMapMemory(m_device, m_uniformBuffers[currentFrame].GetMemory(), 0, sizeof(viewProjectionMatrix), 0, &data);
        memcpy(data, &viewProjectionMatrix, sizeof(viewProjectionMatrix));
        vkUnmapMemory(m_device, m_uniformBuffers[currentFrame].GetMemory());
    }

}
