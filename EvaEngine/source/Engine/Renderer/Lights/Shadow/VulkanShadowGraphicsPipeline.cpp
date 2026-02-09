#include "pch.h"
#include "VulkanShadowGraphicsPipeline.h"
#include "Engine/Platform/Vulkan/VulkanShader.h"
#include "Engine/AssetManager/AssetManager.h"
#include "Engine/Renderer/Renderer2D/VulkanRenderer2D.h"

namespace Engine {

    void VulkanShadowGraphicsPipeline::Init(VkDevice device, VkRenderPass shadowRenderPass)
    {
        m_device = device;

        // Load shadow shaders
        m_3dShadowShader = std::make_shared<VulkanShader>(
            AssetManager::GetAssetPath("shaders/shadow_3d_Vertex.GLSL").string());

        m_tilesShadowShader = std::make_shared<VulkanShader>(
            AssetManager::GetAssetPath("shaders/shadow_tiles_Vertex.GLSL").string());

        m_groundShadowShader = std::make_shared<VulkanShader>(
            AssetManager::GetAssetPath("shaders/shadow_ground_Vertex.GLSL").string());

        EE_CORE_INFO("Shadow pipelines initialized");
    }

    void VulkanShadowGraphicsPipeline::Shutdown(VkDevice device)
    {
        if (m_3dShadowPipeline) {
            vkDestroyPipeline(device, m_3dShadowPipeline, nullptr);
            m_3dShadowPipeline = VK_NULL_HANDLE;
        }
        if (m_3dShadowPipelineLayout) {
            vkDestroyPipelineLayout(device, m_3dShadowPipelineLayout, nullptr);
            m_3dShadowPipelineLayout = VK_NULL_HANDLE;
        }

        if (m_tilesShadowPipeline) {
            vkDestroyPipeline(device, m_tilesShadowPipeline, nullptr);
            m_tilesShadowPipeline = VK_NULL_HANDLE;
        }
        if (m_tilesShadowPipelineLayout) {
            vkDestroyPipelineLayout(device, m_tilesShadowPipelineLayout, nullptr);
            m_tilesShadowPipelineLayout = VK_NULL_HANDLE;
        }

        if (m_groundShadowPipeline) {
            vkDestroyPipeline(device, m_groundShadowPipeline, nullptr);
            m_groundShadowPipeline = VK_NULL_HANDLE;
        }
        if (m_groundShadowPipelineLayout) {
            vkDestroyPipelineLayout(device, m_groundShadowPipelineLayout, nullptr);
            m_groundShadowPipelineLayout = VK_NULL_HANDLE;
        }
    }

    void VulkanShadowGraphicsPipeline::Create3DShadowPipeline(VkDevice device, VkRenderPass shadowRenderPass, VkDescriptorSetLayout set0Layout)
    {
        // Vertex input for 3D meshes
        VkVertexInputBindingDescription binding{};
        binding.binding = 0;
        binding.stride = sizeof(Engine::VulkanRenderer3D::Vertex3D);  // pos(3) + nrm(3) + uv(2) + joints(4) + weights(4)
        binding.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

        using V = Engine::VulkanRenderer3D::Vertex3D;

        std::array<VkVertexInputAttributeDescription, 3> attrs{};

        attrs[0] = { 0, 0, VK_FORMAT_R32G32B32_SFLOAT, (uint32_t)offsetof(V, pos) };
        attrs[1] = { 3, 0, VK_FORMAT_R32G32B32A32_UINT, (uint32_t)offsetof(V, joints) };
        attrs[2] = { 4, 0, VK_FORMAT_R32G32B32A32_SFLOAT, (uint32_t)offsetof(V, weights) };

        binding.stride = sizeof(V);

        VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
        vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
        vertexInputInfo.vertexBindingDescriptionCount = 1;
        vertexInputInfo.pVertexBindingDescriptions = &binding;
        vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attrs.size());
        vertexInputInfo.pVertexAttributeDescriptions = attrs.data();

        // Push constant for light space matrix
        VkPushConstantRange pushConstant{};
        pushConstant.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
        pushConstant.offset = 0;
        pushConstant.size = sizeof(Engine::VulkanRenderer3D::ShadowPC);

        // Use the same descriptor set layout as main pipeline
        VkPipelineLayoutCreateInfo layoutInfo{};
        layoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        layoutInfo.setLayoutCount = 1;
        layoutInfo.pSetLayouts = &set0Layout;  // Reuse main 3D layout!
        layoutInfo.pushConstantRangeCount = 1;
        layoutInfo.pPushConstantRanges = &pushConstant;

        vkCreatePipelineLayout(device, &layoutInfo, nullptr, &m_3dShadowPipelineLayout);

        // Create pipeline
        CreateDepthOnlyPipeline(
            device,
            shadowRenderPass,
            m_3dShadowShader->GetVertexShaderModule(),
            vertexInputInfo,
            m_3dShadowPipelineLayout,
            m_3dShadowPipeline
        );

        EE_CORE_INFO("3D shadow pipeline created");
    }

    void VulkanShadowGraphicsPipeline::CreateTilesShadowPipeline(
        VkDevice device,
        VkRenderPass shadowRenderPass,
        VkDescriptorSetLayout instanceLayout)  // The bindless descriptor layout
    {
        // No vertex input - tiles use gl_VertexIndex and gl_InstanceIndex
        VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
        vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
        vertexInputInfo.vertexBindingDescriptionCount = 0;
        vertexInputInfo.vertexAttributeDescriptionCount = 0;

        // Push constant for light space matrix
        VkPushConstantRange pushConstant{};
        pushConstant.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
        pushConstant.offset = 0;
        pushConstant.size = sizeof(Engine::VulkanBindlessDescriptorSetRenderer::TilePC);

        VkPipelineLayoutCreateInfo layoutInfo{};
        layoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        layoutInfo.setLayoutCount = 1;
        layoutInfo.pSetLayouts = &instanceLayout;  // Need instance buffer access
        layoutInfo.pushConstantRangeCount = 1;
        layoutInfo.pPushConstantRanges = &pushConstant;

        vkCreatePipelineLayout(device, &layoutInfo, nullptr, &m_tilesShadowPipelineLayout);

    
        CreateDepthOnlyTilePipeline(
            device,
            shadowRenderPass,
            m_tilesShadowShader,
            vertexInputInfo,
            m_tilesShadowPipelineLayout,
            m_tilesShadowPipeline
        );

        EE_CORE_INFO("Tiles shadow pipeline created");
    }

    void VulkanShadowGraphicsPipeline::CreateGroundShadowPipeline(VkDevice device, VkRenderPass shadowRenderPass)
    {
        // Vertex input for ground (simple quad vertices)
        VkVertexInputBindingDescription binding{};
        binding.binding = 0;
        binding.stride = sizeof(VulkanQuadVertex);  // Your ground vertex format
        binding.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

        VkVertexInputAttributeDescription attribute{};
        attribute.binding = 0;
        attribute.location = 0;
        attribute.format = VK_FORMAT_R32G32B32_SFLOAT;  // Position only
        attribute.offset = 0;

        VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
        vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
        vertexInputInfo.vertexBindingDescriptionCount = 1;
        vertexInputInfo.pVertexBindingDescriptions = &binding;
        vertexInputInfo.vertexAttributeDescriptionCount = 1;
        vertexInputInfo.pVertexAttributeDescriptions = &attribute;

        // Push constant for light space matrix
        VkPushConstantRange pushConstant{};
        pushConstant.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
        pushConstant.offset = 0;
        pushConstant.size = sizeof(glm::mat4);

        VkPipelineLayoutCreateInfo layoutInfo{};
        layoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        layoutInfo.setLayoutCount = 0; 
        layoutInfo.pSetLayouts = nullptr;
        layoutInfo.pushConstantRangeCount = 1;
        layoutInfo.pPushConstantRanges = &pushConstant;



        if (vkCreatePipelineLayout(device, &layoutInfo, nullptr, &m_groundShadowPipelineLayout) != VK_SUCCESS) {
            EE_CORE_ERROR("Failed to create ground shadow pipeline layout");
            return;
        }

        CreateDepthOnlyPipeline(device, shadowRenderPass,m_groundShadowShader->GetVertexShaderModule(),
            vertexInputInfo, m_groundShadowPipelineLayout, m_groundShadowPipeline
        );

        EE_CORE_INFO("Ground shadow pipeline created");
    }

    void VulkanShadowGraphicsPipeline::CreateDepthOnlyPipeline(VkDevice device, VkRenderPass renderPass, VkShaderModule vertShader,
        const VkPipelineVertexInputStateCreateInfo& vertexInputInfo, VkPipelineLayout pipelineLayout,VkPipeline& outPipeline)
    {
        // Shader stage (vertex only, no fragment)

        VkPipelineShaderStageCreateInfo vertStage{};
        vertStage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        vertStage.stage = VK_SHADER_STAGE_VERTEX_BIT;
        vertStage.module = vertShader;
        vertStage.pName = "main";

        // Input assembly
        VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
        inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
        inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        inputAssembly.primitiveRestartEnable = VK_FALSE;

        // Viewport state (dynamic)
        VkPipelineViewportStateCreateInfo viewportState{};
        viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
        viewportState.viewportCount = 1;
        viewportState.scissorCount = 1;

        VkDynamicState dynamicStates[] = {
            VK_DYNAMIC_STATE_VIEWPORT,
            VK_DYNAMIC_STATE_SCISSOR
        };

        VkPipelineDynamicStateCreateInfo dynamicState{};
        dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
        dynamicState.dynamicStateCount = 2;
        dynamicState.pDynamicStates = dynamicStates;

        // Rasterization
        VkPipelineRasterizationStateCreateInfo rasterizer{};
        rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
        rasterizer.depthClampEnable = VK_FALSE;
        rasterizer.rasterizerDiscardEnable = VK_FALSE;
        rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
        rasterizer.cullMode = VK_CULL_MODE_NONE;  // Front-face culling reduces shadow acne
        rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
        rasterizer.depthBiasEnable = VK_TRUE;  // Depth bias for shadow quality
        rasterizer.depthBiasConstantFactor = 1.25f;
        rasterizer.depthBiasSlopeFactor = 1.75f;
        rasterizer.lineWidth = 1.0f;

        // Multisample
        VkPipelineMultisampleStateCreateInfo multisampling{};
        multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
        multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
        multisampling.sampleShadingEnable = VK_FALSE;

        // Depth stencil
        VkPipelineDepthStencilStateCreateInfo depthStencil{};
        depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
        depthStencil.depthTestEnable = VK_TRUE;
        depthStencil.depthWriteEnable = VK_TRUE;
        depthStencil.depthCompareOp = VK_COMPARE_OP_LESS;
        depthStencil.depthBoundsTestEnable = VK_FALSE;
        depthStencil.stencilTestEnable = VK_FALSE;
     
     
        // No color blend (depth-only pass)
        VkPipelineColorBlendStateCreateInfo colorBlend{};
        colorBlend.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
        colorBlend.attachmentCount = 0;
     
        // Create pipeline
        VkGraphicsPipelineCreateInfo pipelineInfo{};
        pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
        pipelineInfo.stageCount = 1;  // Only vertex shader
        pipelineInfo.pStages = &vertStage;
        pipelineInfo.pVertexInputState = &vertexInputInfo;
        pipelineInfo.pInputAssemblyState = &inputAssembly;
        pipelineInfo.pViewportState = &viewportState;
        pipelineInfo.pRasterizationState = &rasterizer;
        pipelineInfo.pMultisampleState = &multisampling;
        pipelineInfo.pDepthStencilState = &depthStencil;
        pipelineInfo.pColorBlendState = &colorBlend;
        pipelineInfo.pDynamicState = &dynamicState;
        pipelineInfo.layout = pipelineLayout;
        pipelineInfo.renderPass = renderPass;
        pipelineInfo.subpass = 0;

        if (vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &outPipeline) != VK_SUCCESS) {
            EE_CORE_ERROR("Failed to create shadow pipeline");
        }
    }

    void VulkanShadowGraphicsPipeline::CreateDepthOnlyTilePipeline(VkDevice device, VkRenderPass renderPass, Ref<VulkanShader> shader,
        const VkPipelineVertexInputStateCreateInfo& vertexInputInfo, VkPipelineLayout pipelineLayout, VkPipeline& outPipeline)
    {
        // Shader stage (vertex only, no fragment)

        VkPipelineShaderStageCreateInfo vs{ VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO };
        vs.stage = VK_SHADER_STAGE_VERTEX_BIT;
        vs.module = shader->GetVertexShaderModule();
        vs.pName = "main";

        VkPipelineShaderStageCreateInfo fs{ VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO };
        fs.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
        fs.module = shader->GetFragmentShaderModule();
        fs.pName = "main";

        VkPipelineShaderStageCreateInfo shaderStages[2] = { vs, fs };
        // Input assembly
        VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
        inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
        inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        inputAssembly.primitiveRestartEnable = VK_FALSE;

        // Viewport state (dynamic)
        VkPipelineViewportStateCreateInfo viewportState{};
        viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
        viewportState.viewportCount = 1;
        viewportState.scissorCount = 1;

        VkDynamicState dynamicStates[] = {
            VK_DYNAMIC_STATE_VIEWPORT,
            VK_DYNAMIC_STATE_SCISSOR
        };

        VkPipelineDynamicStateCreateInfo dynamicState{};
        dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
        dynamicState.dynamicStateCount = 2;
        dynamicState.pDynamicStates = dynamicStates;

        // Rasterization
        VkPipelineRasterizationStateCreateInfo rasterizer{};
        rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
        rasterizer.depthClampEnable = VK_FALSE;
        rasterizer.rasterizerDiscardEnable = VK_FALSE;
        rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
        rasterizer.cullMode = VK_CULL_MODE_NONE;  // Front-face culling reduces shadow acne
        rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
        rasterizer.depthBiasEnable = VK_FALSE;  // Depth bias for shadow quality
       // rasterizer.depthBiasConstantFactor = 1.25f;
        //rasterizer.depthBiasSlopeFactor = 1.75f;
        rasterizer.depthBiasConstantFactor = 0.01f;
        rasterizer.depthBiasSlopeFactor = 0.01f;
        rasterizer.lineWidth = 1.0f;

        // Multisample
        VkPipelineMultisampleStateCreateInfo multisampling{};
        multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
        multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
        multisampling.sampleShadingEnable = VK_FALSE;

        // Depth stencil
        VkPipelineDepthStencilStateCreateInfo depthStencil{};
        depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
        depthStencil.depthTestEnable = VK_TRUE;
        depthStencil.depthWriteEnable = VK_TRUE;
        depthStencil.depthCompareOp = VK_COMPARE_OP_LESS;
        depthStencil.depthBoundsTestEnable = VK_FALSE;
        depthStencil.stencilTestEnable = VK_FALSE;


        // No color blend (depth-only pass)
        VkPipelineColorBlendStateCreateInfo colorBlend{};
        colorBlend.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
        colorBlend.attachmentCount = 0;

        // Create pipeline
        VkGraphicsPipelineCreateInfo pipelineInfo{};
        pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
        pipelineInfo.stageCount = 2;  // Only vertex shaderf
        pipelineInfo.pStages = shaderStages;
        pipelineInfo.pVertexInputState = &vertexInputInfo;
        pipelineInfo.pInputAssemblyState = &inputAssembly;
        pipelineInfo.pViewportState = &viewportState;
        pipelineInfo.pRasterizationState = &rasterizer;
        pipelineInfo.pMultisampleState = &multisampling;
        pipelineInfo.pDepthStencilState = &depthStencil;
        pipelineInfo.pColorBlendState = &colorBlend;
        pipelineInfo.pDynamicState = &dynamicState;
        pipelineInfo.layout = pipelineLayout;
        pipelineInfo.renderPass = renderPass;
        pipelineInfo.subpass = 0;

        if (vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &outPipeline) != VK_SUCCESS) {
            EE_CORE_ERROR("Failed to create shadow pipeline");
        }
    }
}