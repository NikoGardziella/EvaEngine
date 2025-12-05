#include "pch.h"
#include "Vulkan3DGraphicsPipeline.h"
#include <cstring>

namespace Engine {

    static VkPipelineColorBlendAttachmentState MakeBlendAttachment(bool enable)
    {
        VkPipelineColorBlendAttachmentState a{};
        a.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
            VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
        a.blendEnable = enable ? VK_TRUE : VK_FALSE;
        if (enable) 
        {
            a.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
            a.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
            a.colorBlendOp = VK_BLEND_OP_ADD;
            a.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
            a.dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
            a.alphaBlendOp = VK_BLEND_OP_ADD;
        }
        return a;
    }

    bool Vulkan3DGraphicsPipeline::Create(const CreateInfo& ci,  const ShaderStages& stages,
        const VertexInput& vi,    const RasterState& rs)
    {
        Destroy();

        // Basic validation
        if (!ci.device || !ci.renderPass || !stages.vert || !stages.frag)
        {
            return false;
        }
        m_device = ci.device;
        m_ci = ci;

        // --- Pipeline layout ---
        VkPipelineLayoutCreateInfo plCI{ VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO };
        plCI.setLayoutCount = static_cast<uint32_t>(ci.setLayouts.size());
        plCI.pSetLayouts = ci.setLayouts.empty() ? nullptr : ci.setLayouts.data();

        VkPushConstantRange pc = ci.pushConstantRange;
        if (pc.size > 0)
        {
            plCI.pushConstantRangeCount = 1;
            plCI.pPushConstantRanges = &pc;
        }

        if (vkCreatePipelineLayout(m_device, &plCI, nullptr, &m_layout) != VK_SUCCESS)
        {
            m_layout = VK_NULL_HANDLE;
            return false;
        }

        // --- Shader stages ---
        VkPipelineShaderStageCreateInfo vs{ VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO };
        vs.stage = VK_SHADER_STAGE_VERTEX_BIT;
        vs.module = stages.vert;
        vs.pName = "main";

        VkPipelineShaderStageCreateInfo fs{ VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO };
        fs.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
        fs.module = stages.frag;
        fs.pName = "main";

        VkPipelineShaderStageCreateInfo shaderStages[2] = { vs, fs };

        // --- Vertex input ---
        VkPipelineVertexInputStateCreateInfo viCI{ VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO };
        viCI.vertexBindingDescriptionCount = static_cast<uint32_t>(vi.bindings.size());
        viCI.pVertexBindingDescriptions = vi.bindings.empty() ? nullptr : vi.bindings.data();
        viCI.vertexAttributeDescriptionCount = static_cast<uint32_t>(vi.attributes.size());
        viCI.pVertexAttributeDescriptions = vi.attributes.empty() ? nullptr : vi.attributes.data();

        // --- Input assembly ---
        VkPipelineInputAssemblyStateCreateInfo iaCI{ VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO };
        iaCI.topology = rs.topology;
        iaCI.primitiveRestartEnable = VK_FALSE;

        // --- Viewport/Scissor (dynamic) ---
        VkPipelineViewportStateCreateInfo vpCI{ VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO };
        vpCI.viewportCount = 1;
        vpCI.scissorCount = 1;

        // --- Rasterizer ---
        VkPipelineRasterizationStateCreateInfo rsCI{ VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO };
        rsCI.polygonMode = rs.polygonMode;
        rsCI.cullMode = rs.cullMode;
        rsCI.frontFace = rs.frontFace;
        rsCI.lineWidth = 1.0f;
        rsCI.depthClampEnable = VK_FALSE;
        rsCI.rasterizerDiscardEnable = VK_FALSE;

        // --- Multisample ---
        VkPipelineMultisampleStateCreateInfo msCI{ VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO };
        msCI.rasterizationSamples = ci.msaaSamples;
        msCI.sampleShadingEnable = VK_FALSE;

        // --- Depth/Stencil ---
        VkPipelineDepthStencilStateCreateInfo depthStencil{};
        depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
        depthStencil.depthTestEnable = VK_TRUE;
        depthStencil.depthWriteEnable = VK_TRUE;
        depthStencil.depthCompareOp = VK_COMPARE_OP_LESS;  // or LESS_OR_EQUAL
        depthStencil.depthBoundsTestEnable = VK_FALSE;
        depthStencil.stencilTestEnable = VK_FALSE;

        // --- Color blending ---
        std::vector<VkPipelineColorBlendAttachmentState> atts(ci.colorAttachmentCount);
        for (uint32_t i = 0; i < ci.colorAttachmentCount; ++i)
        {
            atts[i] = MakeBlendAttachment(rs.enableBlending);
        }
        VkPipelineColorBlendStateCreateInfo cbCI{ VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO };
        cbCI.attachmentCount = static_cast<uint32_t>(atts.size());
        cbCI.pAttachments = atts.data();

        // --- Dynamic states: viewport + scissor are dynamic ---
        VkDynamicState dynStates[] = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
        VkPipelineDynamicStateCreateInfo dynCI{ VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO };
        dynCI.dynamicStateCount = 2;
        dynCI.pDynamicStates = dynStates;

        // --- Pipeline create info ---
        VkGraphicsPipelineCreateInfo gpCI{ VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO };
        gpCI.stageCount = 2;
        gpCI.pStages = shaderStages;
        gpCI.pVertexInputState = &viCI;
        gpCI.pInputAssemblyState = &iaCI;
        gpCI.pViewportState = &vpCI;
        gpCI.pRasterizationState = &rsCI;
        gpCI.pMultisampleState = &msCI;
        gpCI.pDepthStencilState = &depthStencil;
        gpCI.pColorBlendState = &cbCI;
        gpCI.pDynamicState = &dynCI;
        gpCI.layout = m_layout;
        gpCI.renderPass = ci.renderPass;
        gpCI.subpass = ci.subpassIndex;
        gpCI.basePipelineHandle = VK_NULL_HANDLE;
        gpCI.basePipelineIndex = -1;

        VkResult res = vkCreateGraphicsPipelines(m_device, ci.pipelineCache, 1, &gpCI, nullptr, &m_pipeline);
        if (res != VK_SUCCESS)
        {
            vkDestroyPipelineLayout(m_device, m_layout, nullptr);
            m_layout = VK_NULL_HANDLE;
            m_pipeline = VK_NULL_HANDLE;
            return false;
        }
        EE_CORE_INFO("[PipelineCreate] renderPass = {}", (void*)ci.renderPass);
        EE_CORE_INFO("[PipelineCreate] subpass   = {}", ci.subpassIndex);

        return true;
    }

    void Vulkan3DGraphicsPipeline::Destroy() {
        if (m_pipeline) {
            vkDestroyPipeline(m_device, m_pipeline, nullptr);
            m_pipeline = VK_NULL_HANDLE;
        }
        if (m_layout) {
            vkDestroyPipelineLayout(m_device, m_layout, nullptr);
            m_layout = VK_NULL_HANDLE;
        }
        m_device = VK_NULL_HANDLE;
    }

    void Vulkan3DGraphicsPipeline::Bind(VkCommandBuffer cmd) const {
        if (m_pipeline)
        {
            vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipeline);
        }
    }

} // namespace Engine
