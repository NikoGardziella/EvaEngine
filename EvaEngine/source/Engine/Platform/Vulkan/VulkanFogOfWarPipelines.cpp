#include "pch.h"
#include "VulkanFogOfWarPipelines.h"
#include <cstring>
#include "Engine/AssetManager/AssetManager.h"


namespace Engine {



    bool VulkanFogOfWarPipelines::Init(const VulkanFogOfWarPipelinesCreateInfo& ci)
    {
        m_device = ci.device;
        CreateFogPipelineLayout(m_device);
        m_vulkanFogOfWarShader = std::make_shared<VulkanShader>(AssetManager::GetAssetPath("shaders/fogOFWar_Shader.GLSL").string());

        m_vs = m_vulkanFogOfWarShader->GetVertexShaderModule();
        m_fs = m_vulkanFogOfWarShader->GetFragmentShaderModule();
        if (!m_vs || !m_fs) return false;

        VkPipelineShaderStageCreateInfo stages[2]{};
        stages[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        stages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
        stages[0].module = m_vs;
        stages[0].pName = "main";

        stages[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        stages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
        stages[1].module = m_fs;
        stages[1].pName = "main";

        VkVertexInputBindingDescription bind{};
        bind.binding = 0;
        bind.stride = sizeof(FogVertex);
        bind.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

        VkVertexInputAttributeDescription attr{};
        attr.location = 0;
        attr.binding = 0;
        attr.format = VK_FORMAT_R32G32_SFLOAT;
        attr.offset = 0;

        VkPipelineVertexInputStateCreateInfo vi{};
        vi.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
        vi.vertexBindingDescriptionCount = 1;
        vi.pVertexBindingDescriptions = &bind;
        vi.vertexAttributeDescriptionCount = 1;  // Just 1 attribute
        vi.pVertexAttributeDescriptions = &attr;

        VkPipelineInputAssemblyStateCreateInfo ia{};
        ia.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
        ia.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        ia.primitiveRestartEnable = VK_FALSE;

        VkPipelineViewportStateCreateInfo vp{};
        vp.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
        vp.viewportCount = 1;
        vp.scissorCount = 1;

        VkPipelineRasterizationStateCreateInfo rs{};
        rs.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
        rs.polygonMode = VK_POLYGON_MODE_FILL;
        rs.cullMode = VK_CULL_MODE_NONE;
        rs.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
        rs.lineWidth = 1.0f;

        VkPipelineMultisampleStateCreateInfo ms{};
        ms.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
        ms.rasterizationSamples = ci.msaaSamples;

        VkPipelineDepthStencilStateCreateInfo dsStencilWrite{};
        dsStencilWrite.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
        dsStencilWrite.depthTestEnable = VK_FALSE;
        dsStencilWrite.depthWriteEnable = VK_FALSE;
        dsStencilWrite.stencilTestEnable = VK_TRUE;  // ENABLE stencil

        VkStencilOpState stWrite{};
        stWrite.compareOp = VK_COMPARE_OP_ALWAYS;
        stWrite.failOp = VK_STENCIL_OP_REPLACE;
        stWrite.passOp = VK_STENCIL_OP_REPLACE;
        stWrite.depthFailOp = VK_STENCIL_OP_REPLACE;
        stWrite.compareMask = 0xFF;
        stWrite.writeMask = 0xFF;
        stWrite.reference = 1;
        dsStencilWrite.front = stWrite;
        dsStencilWrite.back = stWrite;

        // Pipeline for fog overlay - TEST stencil
        VkPipelineDepthStencilStateCreateInfo dsFogOverlay{};
        dsFogOverlay.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
        dsFogOverlay.depthTestEnable = VK_FALSE;
        dsFogOverlay.depthWriteEnable = VK_FALSE;
        dsFogOverlay.stencilTestEnable = VK_TRUE;  // ENABLE stencil

        VkStencilOpState stTest{};
        stTest.compareOp = VK_COMPARE_OP_NOT_EQUAL;  // Draw where stencil != 1
        stTest.failOp = VK_STENCIL_OP_KEEP;
        stTest.passOp = VK_STENCIL_OP_KEEP;
        stTest.depthFailOp = VK_STENCIL_OP_KEEP;
        stTest.compareMask = 0xFF;
        stTest.writeMask = 0x00;
        stTest.reference = 1;
        dsFogOverlay.front = stTest;
        dsFogOverlay.back = stTest;

        // Color blending - no cutout tricks
        VkPipelineColorBlendAttachmentState cbNone{};
        cbNone.colorWriteMask = 0;  // Visibility fan: no color writes
        cbNone.blendEnable = VK_FALSE;

        VkPipelineColorBlendAttachmentState cbAlpha{};
        cbAlpha.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
            VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
        cbAlpha.blendEnable = VK_TRUE;
        cbAlpha.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
        cbAlpha.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
        cbAlpha.colorBlendOp = VK_BLEND_OP_ADD;
        cbAlpha.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
        cbAlpha.dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
        cbAlpha.alphaBlendOp = VK_BLEND_OP_ADD;

        VkPipelineColorBlendStateCreateInfo cb{};
        cb.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
        cb.logicOpEnable = VK_FALSE;
        cb.attachmentCount = 1;

        VkDynamicState dynStates[] = {
        VK_DYNAMIC_STATE_VIEWPORT,
        VK_DYNAMIC_STATE_SCISSOR,

        
        };
        VkPipelineDynamicStateCreateInfo dyn{};
        dyn.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
        dyn.dynamicStateCount = (uint32_t)(sizeof(dynStates) / sizeof(dynStates[0]));
        dyn.pDynamicStates = dynStates;

        VkGraphicsPipelineCreateInfo gp{};
        gp.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
        gp.stageCount = 2;
        gp.pStages = stages;
        gp.pVertexInputState = &vi;
        gp.pInputAssemblyState = &ia;
        gp.pViewportState = &vp;
        gp.pRasterizationState = &rs;
        gp.pMultisampleState = &ms;
        gp.layout = m_pipelineLayout;
        gp.renderPass = ci.renderPass;
        gp.subpass = 0;
        gp.pDynamicState = &dyn;

        // ---------- Pipeline A: stencil write fan ----------
        cb.pAttachments = &cbNone;
        gp.pColorBlendState = &cb;
        gp.pDepthStencilState = &dsStencilWrite;

        if (vkCreateGraphicsPipelines(m_device, VK_NULL_HANDLE, 1, &gp, nullptr, &m_pipeStencilWrite) != VK_SUCCESS)
            return false;

        // ---------- Pipeline B: fog overlay ----------
        cb.pAttachments = &cbAlpha;
        gp.pColorBlendState = &cb;
        gp.pDepthStencilState = &dsFogOverlay;

        if (vkCreateGraphicsPipelines(m_device, VK_NULL_HANDLE, 1, &gp, nullptr, &m_pipeFogOverlay) != VK_SUCCESS)
            return false;

        return true;
    }


    VkPipelineLayout VulkanFogOfWarPipelines::CreateFogPipelineLayout(VkDevice device)
    {
        VkPushConstantRange pc{};
        pc.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
        pc.offset = 0;
        pc.size = sizeof(FogPC);

        VkPipelineLayoutCreateInfo ci{};
        ci.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        ci.setLayoutCount = 0;          // no descriptors
        ci.pSetLayouts = nullptr;
        ci.pushConstantRangeCount = 1;
        ci.pPushConstantRanges = &pc;

        m_pipelineLayout = VK_NULL_HANDLE;
        if (vkCreatePipelineLayout(device, &ci, nullptr, &m_pipelineLayout) != VK_SUCCESS)
            return VK_NULL_HANDLE;

        return m_pipelineLayout;
    }



    void VulkanFogOfWarPipelines::Destroy()
    {
        if (m_pipeFogOverlay) vkDestroyPipeline(m_device, m_pipeFogOverlay, nullptr);
        if (m_pipeStencilWrite) vkDestroyPipeline(m_device, m_pipeStencilWrite, nullptr);

        if (m_fs) vkDestroyShaderModule(m_device, m_fs, nullptr);
        if (m_vs) vkDestroyShaderModule(m_device, m_vs, nullptr);

        m_pipeFogOverlay = VK_NULL_HANDLE;
        m_pipeStencilWrite = VK_NULL_HANDLE;
        m_fs = VK_NULL_HANDLE;
        m_vs = VK_NULL_HANDLE;
        m_device = VK_NULL_HANDLE;
    }

}
