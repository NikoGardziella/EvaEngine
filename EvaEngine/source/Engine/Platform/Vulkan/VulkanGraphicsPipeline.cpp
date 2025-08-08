#include "pch.h"
#include "VulkanGraphicsPipeline.h"
#include "Engine/AssetManager/AssetManager.h"
#include "VulkanBuffer.h"
#include "Engine/Core/Core.h"


#include <stdexcept>
#include <vector>
#include <Engine/Renderer/Shader.h>
#include "VulkanContext.h"
#include <Engine/Renderer/Renderer.h>
#include "VulkanUtils.h"
#include "VulkanTexture.h"
#include <Engine/Map/TextureStreaming/TextureStreamingSystem.h>



namespace Engine {


    VulkanGraphicsPipeline::VulkanGraphicsPipeline(VulkanContext& vulkanContext)

    {
		m_swapchainExtent = vulkanContext.GetVulkanSwapchain().GetSwapchainExtent();
	
		m_device = vulkanContext.GetDeviceManager().GetDevice();
        m_descriptorPool = vulkanContext.GetDescriptorPool();
		m_lineDescriptorPool = vulkanContext.GetLineDescriptorPool();

        m_pixelGameShader = std::make_shared<VulkanShader>(AssetManager::GetAssetPath("shaders/PixelGameShader.GLSL").string());
        m_fullscreenShader = std::make_shared<VulkanShader>(AssetManager::GetAssetPath("shaders/fullscreen_shader.GLSL").string());
        m_lineShader = std::make_shared<VulkanShader>(AssetManager::GetAssetPath("shaders/Line_shader.GLSL").string());
        m_computeShader = std::make_shared<VulkanShader>(AssetManager::GetAssetPath("shaders/compute.comp").string());

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


        m_bulletUniformBuffers.resize(MAX_FRAMES_IN_FLIGHT);
        for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
        {
            m_bulletUniformBuffers[i] = VulkanBuffer(
                m_device,
                vulkanContext.GetDeviceManager().GetPhysicalDevice(),
                sizeof(CollisionEntitiesGPU) * MAX_COLLISION_ENTITIES,
                VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
            );
        }

        m_textureUniformBuffers.resize(MAX_FRAMES_IN_FLIGHT);
        for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
        {
            m_textureUniformBuffers[i] = VulkanBuffer(
                m_device,
                vulkanContext.GetDeviceManager().GetPhysicalDevice(),
                sizeof(TextureInfo),
                VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
            );
        }
      

        

        m_vulkanRenderShader = std::make_shared<VulkanShader>(AssetManager::GetAssetPath("shaders/VulkanRenderer2D_Quad.GLSL").string());
		m_vulkanProjectileRenderShader = std::make_shared<VulkanShader>(AssetManager::GetAssetPath("shaders/VulkanRenderer2D_projectile.GLSL").string());
       
        CreatePresentSampler();
        CreateGPUCollisionResultBuffer();
        CreateBlockedTileMaskBuffer();
        CreateDescriptorSetLayouts();
        CreateProjectileDescriptorSetLayout();
        CreateCameraDescriptorSetLayout();
        CreateComputeArrayDescriptorSetLayout();

        CreateLineGraphicsPipeline(vulkanContext.GetGameRenderPass());

        CreateGameGraphicsPipeline(vulkanContext.GetGameRenderPass());
        CreateComputeGraphicsPipeline(vulkanContext.GetGameRenderPass());
        CreateGameDescriptorSet();
        CreateProjectileDescriptorSet();
        CreateCameraDescriptorSet();
        CreatePresentDescriptorSet();
        CreateLineDescriptorSet();

        CreatePresentGameDescriptorPool();
 
        CreatePresentPipelineLayout();
        CreatePresentGraphicsPipeline(vulkanContext.GetPresentRenderPass());
        CreateProjectileGraphicsPipeline(vulkanContext.GetGameRenderPass());

        CreateComputeDescriptorSet();

        AssetManager::AddTexture("logo", Engine::AssetManager::GetAssetPath("textures/ee_logo.png").string(), false);

        m_whiteTexture = AssetManager::GetTexture("logo");
        m_dummyTexture = std::make_shared<VulkanTexture>(1, 1, VK_FORMAT_R8_UINT);

    }

    VulkanGraphicsPipeline::~VulkanGraphicsPipeline()
    {
        vkDestroyPipeline(m_device, m_gameGraphicsPipeline, nullptr);
        vkDestroyPipeline(m_device, m_presentPipeline, nullptr);
        vkDestroyPipeline(m_device, m_linePipeline, nullptr);
        vkDestroyPipeline(m_device, m_computePipeline, nullptr);
        vkDestroyPipeline(m_device, m_projectilePipeline, nullptr);
        vkDestroyPipelineLayout(m_device, m_gamePipelineLayout, nullptr);
        vkDestroyPipelineLayout(m_device, m_imguiPipelineLayout, nullptr);
        vkDestroyPipelineLayout(m_device, m_linePipelineLayout, nullptr);
        vkDestroyPipelineLayout(m_device, m_presentPipelineLayout, nullptr);
        vkDestroyPipelineLayout(m_device, m_computePipelineLayout, nullptr);
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

        std::array<VkVertexInputBindingDescription, 1> bindingDescriptions{};
        bindingDescriptions[0].binding = 0;
        bindingDescriptions[0].stride = sizeof(VulkanQuadVertex);
        bindingDescriptions[0].inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

        /*
        bindingDescriptions[1].binding = 2;
        bindingDescriptions[1].stride = sizeof(BulletData);
        bindingDescriptions[1].inputRate = VK_VERTEX_INPUT_RATE_INSTANCE;
        bindingDescriptions[2].binding = 4;
        bindingDescriptions[2].stride = sizeof(TextureInfo);
        bindingDescriptions[2].inputRate = VK_VERTEX_INPUT_RATE_INSTANCE;
        */



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
        vertexInputInfo.vertexBindingDescriptionCount = static_cast<uint32_t>(bindingDescriptions.size());
        vertexInputInfo.pVertexBindingDescriptions = bindingDescriptions.data();
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
        colorBlendAttachment.blendEnable = VK_TRUE;  
        colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA; 
        colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA; 
        colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
        colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA; 
        colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA; 
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
        colorBlending.blendConstants[1] = 0.0f;  
        colorBlending.blendConstants[2] = 0.0f;  
        colorBlending.blendConstants[3] = 0.0f;

        
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

    void VulkanGraphicsPipeline::CreateComputeGraphicsPipeline(VkRenderPass renderPass)
    {
        VkPipelineShaderStageCreateInfo computeShaderStageInfo{};
        computeShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        computeShaderStageInfo.stage = VK_SHADER_STAGE_COMPUTE_BIT;
        computeShaderStageInfo.module = m_computeShader->GetComputeshaderModule();
        computeShaderStageInfo.pName = "main";

        VkPushConstantRange pushConstantRange{};
        pushConstantRange.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
        pushConstantRange.offset = 0;
		pushConstantRange.size = sizeof(PushConstants); 


        VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
        pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        pipelineLayoutInfo.setLayoutCount = 1;
        pipelineLayoutInfo.pSetLayouts = &m_computeArrayDescriptorSetLayout;
        pipelineLayoutInfo.pushConstantRangeCount = 1;
        pipelineLayoutInfo.pPushConstantRanges = &pushConstantRange;

        vkCreatePipelineLayout(m_device, &pipelineLayoutInfo, nullptr, &m_computePipelineLayout);

        VkComputePipelineCreateInfo computePipelineInfo{};
        computePipelineInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
        computePipelineInfo.stage = computeShaderStageInfo;
        computePipelineInfo.layout = m_computePipelineLayout;

        vkCreateComputePipelines(m_device, VK_NULL_HANDLE, 1, &computePipelineInfo, nullptr, &m_computePipeline);


    }

    void VulkanGraphicsPipeline::CreatePresentGraphicsPipeline(VkRenderPass renderPass)
    {

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

    void VulkanGraphicsPipeline::CreateProjectileGraphicsPipeline(VkRenderPass renderPass)
    {

        VkPipelineShaderStageCreateInfo vertShaderStageInfo{};
        vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
        vertShaderStageInfo.module = m_vulkanProjectileRenderShader->GetVertexShaderModule();
        vertShaderStageInfo.pName = "main";

        VkPipelineShaderStageCreateInfo fragShaderStageInfo{};
        fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
        fragShaderStageInfo.module = m_vulkanProjectileRenderShader->GetFragmentShaderModule();
        fragShaderStageInfo.pName = "main";

        VkPipelineShaderStageCreateInfo shaderStages[] = { vertShaderStageInfo, fragShaderStageInfo };

        VkPipelineDynamicStateCreateInfo dynamicState{};
        dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
        dynamicState.dynamicStateCount = static_cast<uint32_t>(m_dynamicStates.size());
        dynamicState.pDynamicStates = m_dynamicStates.data();

        std::array<VkVertexInputBindingDescription, 1> bindingDescriptions{};
        bindingDescriptions[0].binding = 0;
        bindingDescriptions[0].stride = sizeof(VulkanProjectileVertex);
        bindingDescriptions[0].inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

       


        // Define the vertex input attribute descriptions
        std::array<VkVertexInputAttributeDescription, 4> attributeDescriptions{};
        attributeDescriptions[0].binding = 0;
        attributeDescriptions[0].location = 0;
        attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
        attributeDescriptions[0].offset = offsetof(VulkanProjectileVertex, Position);

        attributeDescriptions[1].binding = 0;
        attributeDescriptions[1].location = 1;
        attributeDescriptions[1].format = VK_FORMAT_R32G32B32A32_SFLOAT;
        attributeDescriptions[1].offset = offsetof(VulkanProjectileVertex, Color);

        attributeDescriptions[2].binding = 0;
        attributeDescriptions[2].location = 2;
        attributeDescriptions[2].format = VK_FORMAT_R32G32_SFLOAT;
        attributeDescriptions[2].offset = offsetof(VulkanProjectileVertex, TexCoord);

        attributeDescriptions[3].binding = 0;
        attributeDescriptions[3].location = 3;
        attributeDescriptions[3].format = VK_FORMAT_R32_SFLOAT;
        attributeDescriptions[3].offset = offsetof(VulkanProjectileVertex, TexIndex);

       
        //format of the vertex data that will be passed to the vertex shader.
        VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
        vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
        vertexInputInfo.vertexBindingDescriptionCount = static_cast<uint32_t>(bindingDescriptions.size());
        vertexInputInfo.pVertexBindingDescriptions = bindingDescriptions.data();
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
        colorBlendAttachment.blendEnable = VK_TRUE;
        colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
        colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
        colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
        colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
        colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
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
        colorBlending.blendConstants[1] = 0.0f;
        colorBlending.blendConstants[2] = 0.0f;
        colorBlending.blendConstants[3] = 0.0f;


        VkDescriptorSetLayout setLayouts[] = {
			m_cameraDescriptorSetLayout,
            m_projectileDescriptorSetLayout
        };

        VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
        pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        pipelineLayoutInfo.pushConstantRangeCount = 0; // Optional
        pipelineLayoutInfo.pPushConstantRanges = nullptr; // Optional
        pipelineLayoutInfo.setLayoutCount = 2;
        pipelineLayoutInfo.pSetLayouts = setLayouts;


        if (vkCreatePipelineLayout(m_device, &pipelineLayoutInfo, nullptr, &m_projectilePipelineLayout) != VK_SUCCESS)
        {
            EE_CORE_ASSERT(false, "failed to create projectile pipeline layout!");
        }
        else
        {
            EE_CORE_INFO("Vulkan game projectile pipeline layout created");
        }

        VkPipelineDepthStencilStateCreateInfo depthStencil{};
        depthStencil.depthTestEnable = VK_FALSE;
        depthStencil.depthWriteEnable = VK_FALSE;

        VkGraphicsPipelineCreateInfo pipelineInfo{};
        pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;

        pipelineInfo.stageCount = 2;
        pipelineInfo.pStages = shaderStages;
        pipelineInfo.pVertexInputState = &vertexInputInfo;
        pipelineInfo.pInputAssemblyState = &inputAssembly;
        pipelineInfo.pViewportState = &viewportState;
        pipelineInfo.pRasterizationState = &rasterizer;
        pipelineInfo.pMultisampleState = &multisampling;
        pipelineInfo.pDepthStencilState = &depthStencil; // Optional
        pipelineInfo.pColorBlendState = &colorBlending;
        pipelineInfo.pDynamicState = &dynamicState;
        pipelineInfo.layout = m_projectilePipelineLayout;
        pipelineInfo.renderPass = renderPass;
        pipelineInfo.subpass = 0;
        pipelineInfo.basePipelineHandle = VK_NULL_HANDLE; // Optional
        pipelineInfo.basePipelineIndex = -1; // Optional


        if (vkCreateGraphicsPipelines(m_device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &m_projectilePipeline) != VK_SUCCESS)
        {
            EE_CORE_ASSERT(false, "failed to create projectile  graphics pipeline!");
        }
        else
        {
            EE_CORE_INFO("Vulkan projectile  graphics pipeline created");
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
        bindings[1].descriptorCount = MAX_TEXTURES; // 32 textures
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
    /*
    void VulkanGraphicsPipeline::CreateComputeDescriptorSetLayout()
    {
        
        VkDescriptorSetLayoutBinding inputImageBinding{};
        inputImageBinding.binding = 0;
        inputImageBinding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
        inputImageBinding.descriptorCount = 1;
        inputImageBinding.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
        inputImageBinding.pImmutableSamplers = nullptr;

        VkDescriptorSetLayoutBinding outputImageBinding{};
        outputImageBinding.binding = 1;
        outputImageBinding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
        outputImageBinding.descriptorCount = 1;
        outputImageBinding.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
        outputImageBinding.pImmutableSamplers = nullptr;

        VkDescriptorSetLayoutBinding resultBufferBinding{};
        resultBufferBinding.binding = 2;
        resultBufferBinding.descriptorCount = 1;
        resultBufferBinding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        resultBufferBinding.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;


        VkDescriptorSetLayoutBinding healthBinding{};
        healthBinding.binding = 3;
        healthBinding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
        healthBinding.descriptorCount = 1;
        healthBinding.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

        std::array<VkDescriptorSetLayoutBinding, 4> bindings = {
            inputImageBinding, outputImageBinding, resultBufferBinding, healthBinding
        };

        VkDescriptorSetLayoutCreateInfo layoutInfo{};
        layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
        layoutInfo.pBindings = bindings.data();

        vkCreateDescriptorSetLayout(m_device, &layoutInfo, nullptr, &m_computeDescriptorSetLayout);

    }
    */

    void VulkanGraphicsPipeline::CreateComputeArrayDescriptorSetLayout()
    {
        std::array<VkDescriptorSetLayoutBinding, 5> bindings{};

        // Binding 0: input textures
        bindings[0].binding = 0;
        bindings[0].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
        bindings[0].descriptorCount = MAX_TEXTURES;
        bindings[0].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
        bindings[0].pImmutableSamplers = nullptr;

        // Binding 1: health
        bindings[1].binding = 1;
        bindings[1].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
        bindings[1].descriptorCount = MAX_TEXTURES;
        bindings[1].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
        bindings[1].pImmutableSamplers = nullptr;

        bindings[2].binding = 2;
        bindings[2].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        bindings[2].descriptorCount = 1;
        bindings[2].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
        bindings[2].pImmutableSamplers = nullptr;

        // Binding 3: projectile SSBO
        bindings[3].binding = 3;
        bindings[3].descriptorCount = 1;
        bindings[3].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        bindings[3].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
        bindings[3].pImmutableSamplers = nullptr;

        // destroyed tiles mask
        bindings[4].binding = 4;
        bindings[4].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        bindings[4].descriptorCount = 1;
        bindings[4].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
        bindings[4].pImmutableSamplers = nullptr;



        VkDescriptorSetLayoutCreateInfo layoutInfo{};
        layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
        layoutInfo.pBindings = bindings.data();

        VkResult result = vkCreateDescriptorSetLayout(m_device, &layoutInfo, nullptr, &m_computeArrayDescriptorSetLayout);
        EE_CORE_ASSERT(result == VK_SUCCESS, "Failed to create compute descriptor set layout");
    }
    void VulkanGraphicsPipeline::CreateProjectileDescriptorSetLayout()
    {
        VkDescriptorSetLayoutBinding bindings[2] = {};

        // Binding 0: Camera UBO
        bindings[0].binding = 0;
        bindings[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        bindings[0].descriptorCount = 1;
        bindings[0].stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
        bindings[0].pImmutableSamplers = nullptr;

        // Binding 1: Array of Textures
        bindings[1].binding = 1;
        bindings[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        bindings[1].descriptorCount = MAX_PROJECTILES; // usually 32
        bindings[1].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
        bindings[1].pImmutableSamplers = nullptr;

        VkDescriptorSetLayoutCreateInfo layoutInfo{};
        layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        layoutInfo.bindingCount = 2;
        layoutInfo.pBindings = bindings;

        if (vkCreateDescriptorSetLayout(m_device, &layoutInfo, nullptr, &m_projectileDescriptorSetLayout) != VK_SUCCESS)
        {
            EE_CORE_ASSERT(false, "Failed to create projectile descriptor set layout!");
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
    void VulkanGraphicsPipeline::CreateProjectileDescriptorSet()
    {
        m_projectileDescriptorSet.resize(MAX_FRAMES_IN_FLIGHT);

        std::vector<VkDescriptorSetLayout> layouts(MAX_FRAMES_IN_FLIGHT, m_projectileDescriptorSetLayout);

        VkDescriptorSetAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        allocInfo.descriptorPool = m_descriptorPool;
        allocInfo.descriptorSetCount = MAX_FRAMES_IN_FLIGHT;
        allocInfo.pSetLayouts = layouts.data();

        if (vkAllocateDescriptorSets(m_device, &allocInfo, m_projectileDescriptorSet.data()) != VK_SUCCESS)
        {
            EE_CORE_ASSERT("Failed to allocate projectile descriptor sets");
        }

        std::vector<VkWriteDescriptorSet> descriptorWrites;
        descriptorWrites.reserve(MAX_FRAMES_IN_FLIGHT);

        for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
        {
            VkDescriptorBufferInfo bufferInfo{};
            bufferInfo.buffer = m_uniformBuffers[i].GetBuffer();  // Shared UBO
            bufferInfo.offset = 0;
            bufferInfo.range = sizeof(glm::mat4); // Must match shader UBO size

            VkWriteDescriptorSet uboWrite{};
            uboWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            uboWrite.dstSet = m_projectileDescriptorSet[i];
            uboWrite.dstBinding = 0; // Camera UBO
            uboWrite.dstArrayElement = 0;
            uboWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
            uboWrite.descriptorCount = 1;
            uboWrite.pBufferInfo = &bufferInfo;

            descriptorWrites.push_back(uboWrite);

        }

        vkUpdateDescriptorSets(m_device, static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
    }



    void VulkanGraphicsPipeline::CreateLineDescriptorSet()
    {
        m_lineDescriptorSet.resize(MAX_FRAMES_IN_FLIGHT);

        std::vector<VkDescriptorSetLayout> layouts(MAX_FRAMES_IN_FLIGHT, m_lineDescriptorSetLayout);

        VkDescriptorSetAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        allocInfo.descriptorPool = m_lineDescriptorPool;
        allocInfo.descriptorSetCount = MAX_FRAMES_IN_FLIGHT;
        allocInfo.pSetLayouts = layouts.data();

        if (vkAllocateDescriptorSets(m_device, &allocInfo, m_lineDescriptorSet.data()) != VK_SUCCESS)
        {
            throw std::runtime_error("Failed to allocate line descriptor sets!");
        }

        // Assume m_uniformBuffers[i] corresponds to each frame
        std::vector<VkWriteDescriptorSet> descriptorWrites(MAX_FRAMES_IN_FLIGHT);
        VkDescriptorBufferInfo bufferInfo{};

        for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
        {
            bufferInfo.buffer = m_uniformBuffers[i].GetBuffer();  // <- per-frame UBO
            bufferInfo.offset = 0;
            bufferInfo.range = sizeof(glm::mat4); // or whatever your UBO size is

            descriptorWrites[i].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            descriptorWrites[i].dstSet = m_lineDescriptorSet[i];
            descriptorWrites[i].dstBinding = 0;
            descriptorWrites[i].dstArrayElement = 0;
            descriptorWrites[i].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
            descriptorWrites[i].descriptorCount = 1;
            descriptorWrites[i].pBufferInfo = &bufferInfo;
        }

        vkUpdateDescriptorSets(m_device, static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
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

  
    void VulkanGraphicsPipeline::UpdateComputeDescriptorSet(uint32_t frameIndex, std::array<Ref<VulkanTexture>, MAX_TEXTURES> inputTextures, std::array<Ref<VulkanTexture>, MAX_TEXTURES> healthTextures)
    {

        std::vector<VkDescriptorImageInfo> inputImageInfos;
        std::vector<VkDescriptorImageInfo> healthImageInfos;

        inputImageInfos.reserve(inputTextures.size());
        healthImageInfos.reserve(inputTextures.size());

        for (const auto& tex : inputTextures)
        {
            VkDescriptorImageInfo info{};
            info.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
            info.imageView = tex->GetImageView();
            info.sampler = VK_NULL_HANDLE;
            inputImageInfos.push_back(info);
        }

       
        for (const auto& tex : healthTextures)
        {
            VkDescriptorImageInfo info{};
            info.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
            info.imageView = tex->GetImageView();
            info.sampler = VK_NULL_HANDLE;
            healthImageInfos.push_back(info);

        }


        std::vector<VkWriteDescriptorSet> descriptorWrites;

        // Input
        VkWriteDescriptorSet inputWrite{};
        inputWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        inputWrite.dstSet = m_computeDescriptorSet[frameIndex];
        inputWrite.dstBinding = 0;
        inputWrite.descriptorCount = static_cast<uint32_t>(inputImageInfos.size());
        inputWrite.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
        inputWrite.pImageInfo = inputImageInfos.data();
        descriptorWrites.push_back(inputWrite);

       
        // Health (conditionally)
        if (!healthImageInfos.empty())
        {
            VkWriteDescriptorSet healthWrite{};
            healthWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            healthWrite.dstSet = m_computeDescriptorSet[frameIndex];
            healthWrite.dstBinding = 1;
            healthWrite.descriptorCount = static_cast<uint32_t>(healthImageInfos.size());
            healthWrite.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
            healthWrite.pImageInfo = healthImageInfos.data();
            descriptorWrites.push_back(healthWrite);
        }

        vkUpdateDescriptorSets(m_device,
            static_cast<uint32_t>(descriptorWrites.size()),
            descriptorWrites.data(),
            0, nullptr);
    }



    void VulkanGraphicsPipeline::UpdateTrackedImageDescriptorSets(size_t frameIndex, const std::array<Ref<VulkanTexture>, MAX_TEXTURES>& textures)
    {
        std::array<VkDescriptorImageInfo, MAX_TEXTURES> imageInfos{};
        for (uint32_t i = 0; i < MAX_TEXTURES; ++i)
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
    
    void VulkanGraphicsPipeline::UpdateProjectileDescriptorSets(size_t frameIndex, const std::array<Ref<VulkanTexture>, MAX_PROJECTILES>& textures)
    {
        std::array<VkDescriptorImageInfo, MAX_PROJECTILES> imageInfos{};
        for (uint32_t i = 0; i < MAX_PROJECTILES; ++i)
        {
            imageInfos[i].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            imageInfos[i].imageView = textures[i]->GetImageView();
            imageInfos[i].sampler = textures[i]->GetSampler();

        }


        VkWriteDescriptorSet textureWrite{};
        textureWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        textureWrite.dstSet = m_projectileDescriptorSet[frameIndex];
        textureWrite.dstBinding = 1;
        textureWrite.dstArrayElement = 0;
        textureWrite.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        textureWrite.descriptorCount = static_cast<uint32_t>(imageInfos.size());
        textureWrite.pImageInfo = imageInfos.data();

        
        VkDescriptorBufferInfo bufferInfo{};
        bufferInfo.buffer = m_uniformBuffers[frameIndex].GetBuffer();  
        bufferInfo.offset = 0;
        bufferInfo.range = sizeof(glm::mat4); // or your full Camera UBO struct size

        VkWriteDescriptorSet uboWrite{};
        uboWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        uboWrite.dstSet = m_projectileDescriptorSet[frameIndex];
        uboWrite.dstBinding = 0;
        uboWrite.dstArrayElement = 0;
        uboWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        uboWrite.descriptorCount = 1;
        uboWrite.pBufferInfo = &bufferInfo;

        std::array<VkWriteDescriptorSet, 2> descriptorWrites = { uboWrite, textureWrite };
        

        vkUpdateDescriptorSets(m_device, static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
    }

    void VulkanGraphicsPipeline::UpdateTrackedImageDescriptorSets(size_t frameIndex, const std::vector<Ref<VulkanTexture>>& textures)
    {
		
        int textureCount = textures.size();
		if (textureCount <= 0)
		{
			return;
		}

        std::vector<VkDescriptorImageInfo> imageInfos;
		imageInfos.resize(textureCount);
        for (uint32_t i = 0; i < textureCount; ++i)
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


    void VulkanGraphicsPipeline::CreateComputeDescriptorSet()
    {
        m_computeDescriptorSet.resize(MAX_FRAMES_IN_FLIGHT);

        std::vector<VkDescriptorSetLayout> layouts(m_computeDescriptorSet.size(), m_computeArrayDescriptorSetLayout);

        VkDescriptorSetAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        allocInfo.descriptorPool = m_descriptorPool;
        allocInfo.descriptorSetCount = static_cast<uint32_t>(layouts.size());
        allocInfo.pSetLayouts = layouts.data();

        VkResult allocResult = vkAllocateDescriptorSets(m_device, &allocInfo, m_computeDescriptorSet.data());
        EE_CORE_ASSERT(allocResult == VK_SUCCESS, "Failed to allocate compute descriptor sets!");

        for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
        {
     
            VkDescriptorBufferInfo resultBufferInfo{};
            resultBufferInfo.buffer = m_GPUCollisionresultBufferBuffer;
            resultBufferInfo.offset = 0;
            resultBufferInfo.range = sizeof(CollisionResultBuffer);

            VkDescriptorBufferInfo bulletBufferInfo{};
            bulletBufferInfo.buffer = m_bulletUniformBuffers[i].GetBuffer(); 
            bulletBufferInfo.offset = 0;
            bulletBufferInfo.range = sizeof(CollisionEntitiesGPU) * MAX_COLLISION_ENTITIES;

            uint32_t chunkcount = 2;// check this. Is it LOAD_RADIUS from textureSsytem
            VkDescriptorBufferInfo destroyedTileMaskInfo{};
            destroyedTileMaskInfo.buffer = m_blockedTileMaskBuffer;
            destroyedTileMaskInfo.offset = 0;

            uint32_t tilesPerRow = CHUNK_SIZE * CHUNK_GRID_WIDTH * GRID_SUBDIVISIONS;
            uint32_t tilesPerMask = tilesPerRow * tilesPerRow;
            destroyedTileMaskInfo.range = sizeof(uint32_t) * tilesPerMask;

            std::array<VkWriteDescriptorSet, 3> descriptorWrites{};

            // dstBinding = 0 u_InputTexture and 1 u_OutputTexture
            // are updated every frame so no need to do it here

            // Binding 2: result buffer
            descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            descriptorWrites[0].dstSet = m_computeDescriptorSet[i];
            descriptorWrites[0].dstBinding = 2;
            descriptorWrites[0].descriptorCount = 1;
            descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
            descriptorWrites[0].pBufferInfo = &resultBufferInfo;

            //  Binding 3: bullet buffer (SSBO)
            descriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            descriptorWrites[1].dstSet = m_computeDescriptorSet[i];
            descriptorWrites[1].dstBinding = 3;
            descriptorWrites[1].descriptorCount = 1;
            descriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
            descriptorWrites[1].pBufferInfo = &bulletBufferInfo;

            descriptorWrites[2].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            descriptorWrites[2].dstSet = m_computeDescriptorSet[i];
            descriptorWrites[2].dstBinding = 4;
            descriptorWrites[2].descriptorCount = 1;
            descriptorWrites[2].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
            descriptorWrites[2].pBufferInfo = &destroyedTileMaskInfo;



            vkUpdateDescriptorSets(m_device,
                static_cast<uint32_t>(descriptorWrites.size()),
                descriptorWrites.data(),
                0, nullptr);
        }
    }



    void VulkanGraphicsPipeline::CreateGPUCollisionResultBuffer()
    {
        VkBufferCreateInfo bufferInfo{};
        bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        bufferInfo.size = sizeof(CollisionResultBuffer);
        bufferInfo.usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
        bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        vkCreateBuffer(m_device, &bufferInfo, nullptr, &m_GPUCollisionresultBufferBuffer);

        // Allocate memory (host visible + coherent)
        VkMemoryRequirements memRequirements;
        vkGetBufferMemoryRequirements(m_device, m_GPUCollisionresultBufferBuffer, &memRequirements);

        VkMemoryAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        allocInfo.allocationSize = memRequirements.size; 
        allocInfo.memoryTypeIndex = VulkanContext::Get()->FindMemoryType(memRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

        vkAllocateMemory(m_device, &allocInfo, nullptr, &m_GPUCollisionresultBufferMemory);
        vkBindBufferMemory(m_device, m_GPUCollisionresultBufferBuffer, m_GPUCollisionresultBufferMemory, 0);

    }

    void VulkanGraphicsPipeline::CreateBlockedTileMaskBuffer()
    {
        VkDevice device = m_device;
        VkBufferCreateInfo bufferInfo{};
        bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;

        uint32_t tilesPerRow = CHUNK_SIZE * CHUNK_GRID_WIDTH * GRID_SUBDIVISIONS;
        uint32_t tilesPerMask = tilesPerRow * tilesPerRow;
        bufferInfo.size = sizeof(uint32_t) * tilesPerMask;
        bufferInfo.usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
        bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        if (vkCreateBuffer(device, &bufferInfo, nullptr, &m_blockedTileMaskBuffer) != VK_SUCCESS)
            throw std::runtime_error("Failed to create destroyed tile mask buffer");

        VkMemoryRequirements memRequirements;
        vkGetBufferMemoryRequirements(device, m_blockedTileMaskBuffer, &memRequirements);

        VkMemoryAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        allocInfo.allocationSize = memRequirements.size;
        allocInfo.memoryTypeIndex = VulkanContext::Get()->FindMemoryType(
            memRequirements.memoryTypeBits,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
        );

        if (vkAllocateMemory(device, &allocInfo, nullptr, &m_blockedTileMaskMemory) != VK_SUCCESS)
            throw std::runtime_error("Failed to allocate destroyed tile mask memory");

        vkBindBufferMemory(device, m_blockedTileMaskBuffer, m_blockedTileMaskMemory, 0);
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

    void VulkanGraphicsPipeline::UpdateBulletUBODescriptorSets()
    {
        for (size_t i = 0; i < m_gameDescriptorSets.size(); ++i)
        {
            VkDescriptorBufferInfo bulletBufferInfo{};
            bulletBufferInfo.buffer = m_bulletUniformBuffers[i].GetBuffer(); // Per-frame bullet UBO
            bulletBufferInfo.offset = 0;
            bulletBufferInfo.range = VK_WHOLE_SIZE; // Or use sizeof if it's fixed size

            VkWriteDescriptorSet bulletWrite{};
            bulletWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            bulletWrite.dstSet = m_gameDescriptorSets[i]; // Per-frame descriptor set
            bulletWrite.dstBinding = 2;                      // Bullet UBO is at binding = 2
            bulletWrite.dstArrayElement = 0;
            bulletWrite.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
            bulletWrite.descriptorCount = 1;
            bulletWrite.pBufferInfo = &bulletBufferInfo;

            vkUpdateDescriptorSets(m_device, 1, &bulletWrite, 0, nullptr);
        }
    }

    void VulkanGraphicsPipeline::UpdateTextureInfoDescriptorSets()
    {
        for (size_t i = 0; i < m_gameDescriptorSets.size(); ++i)
        {
            VkDescriptorBufferInfo bufferInfo{};
            bufferInfo.buffer = m_textureUniformBuffers[i].GetBuffer();
            bufferInfo.offset = 0;
            bufferInfo.range = sizeof(TextureInfo);

            VkWriteDescriptorSet write{};
            write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            write.dstSet = m_gameDescriptorSets[i];  // Set 1
            write.dstBinding = 4;
            write.dstArrayElement = 0;
            write.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
            write.descriptorCount = 1;
            write.pBufferInfo = &bufferInfo;

            vkUpdateDescriptorSets(m_device, 1, &write, 0, nullptr);
        }
    }

    
    StorageImage VulkanGraphicsPipeline::CreateStorageImage(VkDevice device, VkPhysicalDevice physicalDevice,
        uint32_t width, uint32_t height, VkFormat format,
        VkCommandPool commandPool, VkQueue graphicsQueue) {

        StorageImage result{};

        // 1. Create VkImage
        VkImageCreateInfo imageInfo{};
        imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        imageInfo.imageType = VK_IMAGE_TYPE_2D;
        imageInfo.extent.width = width;
        imageInfo.extent.height = height;
        imageInfo.extent.depth = 1;
        imageInfo.mipLevels = 1;
        imageInfo.arrayLayers = 1;
        imageInfo.format = format; // e.g., VK_FORMAT_R8G8B8A8_UNORM
        imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
        imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        imageInfo.usage = VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
        imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
        imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;


        if (vkCreateImage(device, &imageInfo, nullptr, &result.Image) != VK_SUCCESS)
        {
            EE_CORE_ASSERT("failed to create storage image");
        }
        // 2. Allocate memory
        VkMemoryRequirements memRequirements;
        vkGetImageMemoryRequirements(device, result.Image, &memRequirements);

        VkMemoryAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        allocInfo.allocationSize = memRequirements.size;
        allocInfo.memoryTypeIndex = VulkanContext::Get()->FindMemoryType(
            memRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

        if (vkAllocateMemory(device, &allocInfo, nullptr, &result.Memory) != VK_SUCCESS)
        {
            EE_CORE_ASSERT("failedto allocate memory of storage image");
        }

        if (vkBindImageMemory(device, result.Image, result.Memory, 0) != VK_SUCCESS)
        {
            EE_CORE_ASSERT("failed to bind image memory of storage image");

        }

        // 3. Create image view
        VkImageViewCreateInfo viewInfo{};
        viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        viewInfo.image = result.Image;
        viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        viewInfo.format = format;
        viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        viewInfo.subresourceRange.baseMipLevel = 0;
        viewInfo.subresourceRange.levelCount = 1;
        viewInfo.subresourceRange.baseArrayLayer = 0;
        viewInfo.subresourceRange.layerCount = 1;

        if (vkCreateImageView(device, &viewInfo, nullptr, &result.ImageView) != VK_SUCCESS)
        {
            EE_CORE_ASSERT("failed to create storage image view");
        }

        
        // 4. Transition to GENERAL layout for shader write access
        VulkanUtils::TransitionImageLayout(
            result.Image,
            VK_FORMAT_R8G8B8A8_UNORM,
            VK_IMAGE_LAYOUT_UNDEFINED,
            VK_IMAGE_LAYOUT_GENERAL
        );
        
        result.Height = height;
        result.Width = width;
        return result;
    }



    void VulkanGraphicsPipeline::UpdateCameraUniformBuffer(uint32_t currentFrame, const glm::mat4& viewProjectionMatrix)
    {
        void* data;
        vkMapMemory(m_device, m_uniformBuffers[currentFrame].GetMemory(), 0, sizeof(viewProjectionMatrix), 0, &data);
        memcpy(data, &viewProjectionMatrix, sizeof(viewProjectionMatrix));
        vkUnmapMemory(m_device, m_uniformBuffers[currentFrame].GetMemory());
    }

    void VulkanGraphicsPipeline::UpdateCollisionUniformBuffer(uint32_t currentFrame, const std::array<CollisionEntitiesGPU,MAX_COLLISION_ENTITIES> collidingEntityData)
    {
        void* data;
        VkDeviceSize size = sizeof(CollisionEntitiesGPU) * collidingEntityData.size();
        if (size <= 0) // this probably does not work
        {
            return;
        }

      

        // Map buffer memory
        vkMapMemory(m_device, m_bulletUniformBuffers[currentFrame].GetMemory(),
            0, size, 0, &data);

        memcpy(data, collidingEntityData.data(), size);

        vkUnmapMemory(m_device, m_bulletUniformBuffers[currentFrame].GetMemory());
    }


    void VulkanGraphicsPipeline::UpdateTextureUniformBuffer(uint32_t currentFrame, const glm::ivec2& textureSize)
    {
        TextureInfo textureInfo = { textureSize };

        void* data;
        vkMapMemory(m_device, m_textureUniformBuffers[currentFrame].GetMemory(), 0, sizeof(TextureInfo), 0, &data);
        memcpy(data, &textureInfo, sizeof(TextureInfo));
        vkUnmapMemory(m_device, m_textureUniformBuffers[currentFrame].GetMemory());
    }
}
