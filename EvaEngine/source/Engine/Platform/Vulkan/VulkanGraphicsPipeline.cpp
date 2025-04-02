#include "pch.h"
#include "VulkanGraphicsPipeline.h"
#include "Engine/AssetManager/AssetManager.h"


#include <fstream>
#include <stdexcept>
#include <vector>
#include <Engine/Renderer/Shader.h>
#include "VulkanShader.h"

namespace Engine {

    VulkanGraphicsPipeline::VulkanGraphicsPipeline(VkDevice device, VkExtent2D swapchainExtent, VkRenderPass renderPass)
        : m_device(device), m_graphicsPipeline(VK_NULL_HANDLE), m_pipelineLayout(VK_NULL_HANDLE)
    {
        CreateGraphicsPipeline(swapchainExtent, renderPass);
    }

    VulkanGraphicsPipeline::~VulkanGraphicsPipeline()
    {
        vkDestroyPipeline(m_device, m_graphicsPipeline, nullptr);
        vkDestroyPipelineLayout(m_device, m_pipelineLayout, nullptr);
    }

    void VulkanGraphicsPipeline::CreateGraphicsPipeline(VkExtent2D swapchainExtent, VkRenderPass renderPass)
    {
        // Load shader code
        //std::vector<char> vertShaderCode = AssetManager::ReadFile(AssetManager::GetAssetPath("cache/shader/opengl/testShader.glsl.cached_opengl.vert").string());
        //std::vector<char> fragShaderCode = AssetManager::ReadFile(AssetManager::GetAssetPath("cache/shader/opengl/testShader.glsl.cached_opengl.frag").string());


       // CreateShaderModule(vertShaderCode, &vertShaderModule);
       // CreateShaderModule(fragShaderCode, &fragShaderModule);

        // Ref<Shader> CircleShader = Shader::Create("circle shader",AssetManager::GetAssetPath("cache/shader/opengl/testShader.glsl.cached_opengl.vert").string(), AssetManager::GetAssetPath("cache/shader/opengl/testShader.glsl.cached_opengl.frag").string());
         //Ref<Shader> CircleShader = Shader::Create("circle shader",AssetManager::GetAssetPath("shaders/quadshader.vert").string(), AssetManager::GetAssetPath("shaders/quadshader.frag").string());
         Ref<VulkanShader> CircleShader = std::make_shared<VulkanShader>(AssetManager::GetAssetPath("shaders/Renderer2D_Quad.GLSL").string());


        VkPipelineShaderStageCreateInfo vertShaderStageInfo{};
        vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
        vertShaderStageInfo.module = CircleShader->GetVertexShaderModule();
        vertShaderStageInfo.pName = "main";

        VkPipelineShaderStageCreateInfo fragShaderStageInfo{};
        fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
        fragShaderStageInfo.module = CircleShader->GetFragmentShaderModule();
        fragShaderStageInfo.pName = "main";

        VkPipelineShaderStageCreateInfo shaderStages[] = { vertShaderStageInfo, fragShaderStageInfo };

        // Vertex input state
        VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
        vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
        vertexInputInfo.vertexBindingDescriptionCount = 0;
        vertexInputInfo.pVertexBindingDescriptions = nullptr;
        vertexInputInfo.vertexAttributeDescriptionCount = 0;
        vertexInputInfo.pVertexAttributeDescriptions = nullptr;

        // Input assembly state
        VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
        inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
        inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        inputAssembly.primitiveRestartEnable = VK_FALSE;

        // Viewport and scissor
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

        // Rasterization state
        VkPipelineRasterizationStateCreateInfo rasterizer{};
        rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
        rasterizer.depthClampEnable = VK_FALSE;
        rasterizer.rasterizerDiscardEnable = VK_FALSE;
        rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
        rasterizer.lineWidth = 1.0f;
        rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
        rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE;
        rasterizer.depthBiasEnable = VK_FALSE;

        // Multisample state
        VkPipelineMultisampleStateCreateInfo multisampling{};
        multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
        multisampling.sampleShadingEnable = VK_FALSE;
        multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

        // Color blend state
        VkPipelineColorBlendAttachmentState colorBlendAttachment{};
        colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
        colorBlendAttachment.blendEnable = VK_FALSE;

        VkPipelineColorBlendStateCreateInfo colorBlending{};
        colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
        colorBlending.logicOpEnable = VK_FALSE;
        colorBlending.logicOp = VK_LOGIC_OP_COPY;
        colorBlending.attachmentCount = 1;
        colorBlending.pAttachments = &colorBlendAttachment;
        colorBlending.blendConstants[0] = 0.0f;
        colorBlending.blendConstants[1] = 0.0f;
        colorBlending.blendConstants[2] = 0.0f;
        colorBlending.blendConstants[3] = 0.0f;

        // Pipeline layout creation
        VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
        pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;

        if (vkCreatePipelineLayout(m_device, &pipelineLayoutInfo, nullptr, &m_pipelineLayout) != VK_SUCCESS)
        {
            EE_CORE_ASSERT(false, "Failed to create pipeline layout!");
        }
        else
        {
            EE_CORE_INFO("Vulkan pipeline layout created");
        }

        // Graphics pipeline creation
        VkGraphicsPipelineCreateInfo pipelineInfo{};
        pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
        pipelineInfo.stageCount = 2;
        pipelineInfo.pStages = shaderStages;
        pipelineInfo.pVertexInputState = &vertexInputInfo;
        pipelineInfo.pInputAssemblyState = &inputAssembly;
        pipelineInfo.pViewportState = &viewportState;
        pipelineInfo.pRasterizationState = &rasterizer;
        pipelineInfo.pMultisampleState = &multisampling;
        pipelineInfo.pColorBlendState = &colorBlending;
        pipelineInfo.layout = m_pipelineLayout;
        pipelineInfo.renderPass = renderPass;
        pipelineInfo.subpass = 0;

        if (vkCreateGraphicsPipelines(m_device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &m_graphicsPipeline) != VK_SUCCESS)
        {
            EE_CORE_ASSERT(false, "failed to create graphics pipeline!");
        }
        else
        {
            EE_CORE_INFO("Vulkan graphics pipeline created");
        }

        vkDestroyShaderModule(m_device, CircleShader->GetVertexShaderModule(), nullptr);
        vkDestroyShaderModule(m_device, CircleShader->GetFragmentShaderModule(), nullptr);
    }

  

}
