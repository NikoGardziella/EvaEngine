#include "pch.h"
#include "VulkanFogOfWarPipelines.h"
#include <cstring>
#include "Engine/AssetManager/AssetManager.h"
#include "Engine/Platform/Vulkan/VulkanUtils.h"


namespace Engine {


    bool VulkanFogOfWarPipelines::Init(const VulkanFogOfWarPipelinesCreateInfo& ci)
    {
        m_device = ci.device;
        m_renderPass = ci.renderPass; // Main Pass

        // 1. Create Texture & Layouts
        uint32_t fogSize = 512;
        VkImageUsageFlags usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
        m_fogOfwarTexture = std::make_shared<VulkanTexture>(fogSize, fogSize, usage, VK_FORMAT_R8_UNORM);
        VulkanUtils::TransitionImageLayout(
            m_fogOfwarTexture->GetImage(),
            VK_FORMAT_R8_UNORM,
            VK_IMAGE_LAYOUT_UNDEFINED,
            VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
        );
        CreateFogPipelineLayouts(m_device);
        CreateVisibilityResources(); // Create FB and RenderPass

        // 2. Load Shaders
        m_vulkanFogOfWarShader = std::make_shared<VulkanShader>(AssetManager::GetAssetPath("shaders/fogOFWar_Shader.GLSL").string());
        m_vulkanFogOfWarWriterShader = std::make_shared<VulkanShader>(AssetManager::GetAssetPath("shaders/fog_writer_shader.GLSL").string());

        // 3. Setup Pipelines independently
        if (!CreateVisibilityPipeline(ci)) return false;


        if (!CreateOverlayPipeline(ci))   return false;

        // 4. Finalize Descriptors
        CreateDescriptorPool();
        UpdateDescriptorSet();

        return true;
    }
    bool VulkanFogOfWarPipelines::CreateOverlayPipeline(const VulkanFogOfWarPipelinesCreateInfo& ci)
    {
        // 1. Shader Stages
        VkPipelineShaderStageCreateInfo stages[2]{};
        stages[0] = { VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO };
        stages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
        stages[0].module = m_vulkanFogOfWarShader->GetVertexShaderModule();
        stages[0].pName = "main";

        stages[1] = { VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO };
        stages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
        stages[1].module = m_vulkanFogOfWarShader->GetFragmentShaderModule();
        stages[1].pName = "main";

        // 2. Vertex Input (Empty for full-screen triangle)
        VkPipelineVertexInputStateCreateInfo vi{ VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO };
        vi.vertexAttributeDescriptionCount = 0;
        vi.vertexBindingDescriptionCount = 0;

        // 3. Input Assembly
        VkPipelineInputAssemblyStateCreateInfo ia{ VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO };
        ia.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

        // 4. Viewport State (Required: Fixes VUID-09024)
        VkPipelineViewportStateCreateInfo vp{ VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO };
        vp.viewportCount = 1;
        vp.pViewports = nullptr; // Ignored due to dynamic state
        vp.scissorCount = 1;
        vp.pScissors = nullptr;  // Ignored due to dynamic state

        // 5. Rasterization
        VkPipelineRasterizationStateCreateInfo rs{ VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO };
        rs.polygonMode = VK_POLYGON_MODE_FILL;
        rs.cullMode = VK_CULL_MODE_NONE;
        rs.frontFace = VK_FRONT_FACE_CLOCKWISE;
        rs.lineWidth = 1.0f;

        // 6. Multisample State (Required: Fixes VUID-09026)
        VkPipelineMultisampleStateCreateInfo ms{ VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO };
        ms.rasterizationSamples = ci.msaaSamples; // Must match the renderPass samples
        ms.sampleShadingEnable = VK_FALSE;

        // 7. Depth/Stencil State (Required: Fixes VUID-09028)
        VkPipelineDepthStencilStateCreateInfo ds{ VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO };
        ds.depthTestEnable = VK_FALSE;
        ds.depthWriteEnable = VK_FALSE;
        ds.depthCompareOp = VK_COMPARE_OP_ALWAYS;

        // 8. Color Blend Attachment (Alpha Blending)
        VkPipelineColorBlendAttachmentState cbAlpha{};
        cbAlpha.colorWriteMask = 0xF;
        cbAlpha.blendEnable = VK_TRUE;
        cbAlpha.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
        cbAlpha.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
        cbAlpha.colorBlendOp = VK_BLEND_OP_ADD;
        cbAlpha.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
        cbAlpha.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
        cbAlpha.alphaBlendOp = VK_BLEND_OP_ADD;

        VkPipelineColorBlendStateCreateInfo cbState{ VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO };
        cbState.attachmentCount = 1;
        cbState.pAttachments = &cbAlpha;

        // 9. Dynamic State
        VkDynamicState dStates[] = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
        VkPipelineDynamicStateCreateInfo dyn{ VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO };
        dyn.dynamicStateCount = 2;
        dyn.pDynamicStates = dStates;

        // 10. Pipeline Creation
        VkGraphicsPipelineCreateInfo gp{ VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO };
        gp.stageCount = 2;
        gp.pStages = stages;
        gp.pVertexInputState = &vi;
        gp.pInputAssemblyState = &ia;
        gp.pViewportState = &vp;          // Linked
        gp.pRasterizationState = &rs;
        gp.pMultisampleState = &ms;       // Linked
        gp.pDepthStencilState = &ds;      // Linked
        gp.pColorBlendState = &cbState;
        gp.pDynamicState = &dyn;          // Linked
        gp.layout = m_pipelineLayout;
        gp.renderPass = m_renderPass;     // Main Screen Pass

        return vkCreateGraphicsPipelines(m_device, VK_NULL_HANDLE, 1, &gp, nullptr, &m_pipeFogOverlay) == VK_SUCCESS;
    }

    bool VulkanFogOfWarPipelines::CreateVisibilityPipeline(const VulkanFogOfWarPipelinesCreateInfo& ci)
    {
        // 1. Shader Stages
        VkPipelineShaderStageCreateInfo stages[2]{};
        stages[0] = { VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO };
        stages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
        stages[0].module = m_vulkanFogOfWarWriterShader->GetVertexShaderModule();
        stages[0].pName = "main";

        stages[1] = { VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO };
        stages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
        stages[1].module = m_vulkanFogOfWarWriterShader->GetFragmentShaderModule();
        stages[1].pName = "main";

        
        VkVertexInputBindingDescription binding{};
        binding.binding = 0;
        binding.stride = sizeof(VulkanFogOfWarPipelines::FogVertex);
        binding.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

        VkVertexInputAttributeDescription attr{};
        attr.location = 0;
        attr.binding = 0;
        attr.format = VK_FORMAT_R32G32B32_SFLOAT; // Matches vec3 a_Position
        attr.offset = offsetof(VulkanFogOfWarPipelines::FogVertex, pos); // Assuming .pos is the name

        VkPipelineVertexInputStateCreateInfo vi{ VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO };
        vi.vertexBindingDescriptionCount = 1;
        vi.pVertexBindingDescriptions = &binding;
        vi.vertexAttributeDescriptionCount = 1;
        vi.pVertexAttributeDescriptions = &attr;

        // 3. Input Assembly
        VkPipelineInputAssemblyStateCreateInfo ia{ VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO };
        ia.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

        // 4. Viewport State
        VkPipelineViewportStateCreateInfo vp{ VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO };
        vp.viewportCount = 1;
        vp.scissorCount = 1;

        // 5. Rasterization
        VkPipelineRasterizationStateCreateInfo rs{ VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO };
        rs.polygonMode = VK_POLYGON_MODE_FILL;
        rs.cullMode = VK_CULL_MODE_NONE;
        rs.lineWidth = 1.0f;

        // 6. Multisample (Must be 1 for R8 texture)
        VkPipelineMultisampleStateCreateInfo ms{ VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO };
        ms.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

        // 7. Depth/Stencil (Disabled but required)
        VkPipelineDepthStencilStateCreateInfo ds{ VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO };
        ds.depthTestEnable = VK_FALSE;
        ds.depthWriteEnable = VK_FALSE;

        // 8. Color Blend (R-channel only)
        VkPipelineColorBlendAttachmentState cbAttachment{};
        cbAttachment.colorWriteMask = 0xF;
        cbAttachment.blendEnable = VK_FALSE;

        VkPipelineColorBlendStateCreateInfo cb{ VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO };
        cb.attachmentCount = 1;
        cb.pAttachments = &cbAttachment;

        // 9. Dynamic State
        VkDynamicState dStates[] = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
        VkPipelineDynamicStateCreateInfo dyn{ VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO };
        dyn.dynamicStateCount = 2;
        dyn.pDynamicStates = dStates;

        // 10. Pipeline Create Info
        VkGraphicsPipelineCreateInfo gp{ VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO };
        gp.stageCount = 2;
        gp.pStages = stages;
        gp.pVertexInputState = &vi;
        gp.pInputAssemblyState = &ia;
        gp.pViewportState = &vp;
        gp.pRasterizationState = &rs;
        gp.pMultisampleState = &ms;
        gp.pDepthStencilState = &ds;
        gp.pColorBlendState = &cb;
        gp.pDynamicState = &dyn;
        gp.layout = m_visibilityPipelineLayout;
        gp.renderPass = m_visibilityRenderPass;

        return vkCreateGraphicsPipelines(m_device, VK_NULL_HANDLE, 1, &gp, nullptr, &m_pipeVisibilityMap) == VK_SUCCESS;
    }

    void VulkanFogOfWarPipelines::CreateFogPipelineLayouts(VkDevice device)
    {
        // 1. Define Push Constants (Used by both)
        VkPushConstantRange pcRange{ VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(FogPC) };

        // 2. Create the Descriptor Set Layout FIRST
        VkDescriptorSetLayoutBinding binding{ 0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr };
        VkDescriptorSetLayoutCreateInfo dslCi{ VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO, nullptr, 0, 1, &binding };

        // Ensure m_descriptorSetLayout is a valid handle before proceeding
        if (vkCreateDescriptorSetLayout(device, &dslCi, nullptr, &m_descriptorSetLayout) != VK_SUCCESS) {
            // Handle error
        }

        // 3. Create Layout A: Visibility Writer (0 sets)
        VkPipelineLayoutCreateInfo visLayoutCi{ VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO };
        visLayoutCi.setLayoutCount = 0;          // Correct: No sets for writer
        visLayoutCi.pSetLayouts = nullptr;       // Correct: nullptr is fine when count is 0
        visLayoutCi.pushConstantRangeCount = 1;
        visLayoutCi.pPushConstantRanges = &pcRange;
        vkCreatePipelineLayout(device, &visLayoutCi, nullptr, &m_visibilityPipelineLayout);

        // 4. Create Layout B: Fog Overlay (1 set)
        VkPipelineLayoutCreateInfo overlayLayoutCi{ VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO };
        overlayLayoutCi.setLayoutCount = 1;      // Correct: 1 set for reader
        overlayLayoutCi.pSetLayouts = &m_descriptorSetLayout; // This is now a VALID handle
        overlayLayoutCi.pushConstantRangeCount = 1;
        overlayLayoutCi.pPushConstantRanges = &pcRange;
        vkCreatePipelineLayout(device, &overlayLayoutCi, nullptr, &m_pipelineLayout);
    }

    void VulkanFogOfWarPipelines::CreateDescriptorPool()
    {
        // We only need 1 descriptor of type COMBINED_IMAGE_SAMPLER
        VkDescriptorPoolSize poolSize{};
        poolSize.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        poolSize.descriptorCount = 1;

        VkDescriptorPoolCreateInfo poolInfo{};
        poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        poolInfo.poolSizeCount = 1;
        poolInfo.pPoolSizes = &poolSize;
        poolInfo.maxSets = 1; // We are only allocating 1 set for the fog texture

        if (vkCreateDescriptorPool(m_device, &poolInfo, nullptr, &m_descriptorPool) != VK_SUCCESS)
        {
            EE_CORE_ASSERT(false, "Failed to create Fog Descriptor Pool!");
        }
    }

    void VulkanFogOfWarPipelines::UpdateDescriptorSet()
    {
        

        // 1. Allocate the Descriptor Set
        VkDescriptorSetAllocateInfo allocInfo{ VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO };
        allocInfo.descriptorPool = m_descriptorPool;
        allocInfo.descriptorSetCount = 1;
        allocInfo.pSetLayouts = &m_descriptorSetLayout;

        if (vkAllocateDescriptorSets(m_device, &allocInfo, &m_descriptorSet) != VK_SUCCESS) {
            EE_CORE_ERROR("Failed to allocate Fog Descriptor Set");
            return;
        }

        // 2. Update the Set to point to our R8 Texture
        VkDescriptorImageInfo imageInfo{};
        imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        imageInfo.imageView = m_fogOfwarTexture->GetImageView();
        imageInfo.sampler = m_fogOfwarTexture->GetSampler();

        VkWriteDescriptorSet descriptorWrite{ VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET };
        descriptorWrite.dstSet = m_descriptorSet;
        descriptorWrite.dstBinding = 0;
        descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        descriptorWrite.descriptorCount = 1;
        descriptorWrite.pImageInfo = &imageInfo;

        vkUpdateDescriptorSets(m_device, 1, &descriptorWrite, 0, nullptr);
        
    }



    void VulkanFogOfWarPipelines::CreateVisibilityResources()
    {
        VkAttachmentDescription attachment{};
        attachment.format = VK_FORMAT_R8_UNORM;
        attachment.samples = VK_SAMPLE_COUNT_1_BIT;
        attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;    // MUST BE CLEAR
        attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;  // MUST BE STORE
        attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        attachment.finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL; // Let Vulkan handle the transition

        VkAttachmentReference colorReference = { 0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL };

        VkSubpassDescription subpass{};
        subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
        subpass.colorAttachmentCount = 1;
        subpass.pColorAttachments = &colorReference;

        // Ensure this pass finishes before anything tries to read the texture
        VkSubpassDependency dependency{};
        dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
        dependency.dstSubpass = 0;
        dependency.srcStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
        dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        dependency.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
        dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

        // Add a second dependency for the exit
        VkSubpassDependency dependencyExit{};
        dependencyExit.srcSubpass = 0;
        dependencyExit.dstSubpass = VK_SUBPASS_EXTERNAL;
        dependencyExit.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        dependencyExit.dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
        dependencyExit.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
        dependencyExit.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

        VkSubpassDependency deps[] = { dependency, dependencyExit };

        VkRenderPassCreateInfo rpCi{ VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO };
        rpCi.attachmentCount = 1;
        rpCi.pAttachments = &attachment;
        rpCi.subpassCount = 1;
        rpCi.pSubpasses = &subpass;
        rpCi.dependencyCount = 2;
        rpCi.pDependencies = deps;

        vkCreateRenderPass(m_device, &rpCi, nullptr, &m_visibilityRenderPass);
        // 2. Create the Framebuffer linking the new RenderPass
        VkFramebufferCreateInfo framebufferInfo{ VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO };
        framebufferInfo.renderPass = m_visibilityRenderPass; // USE THE NEW PASS
        framebufferInfo.attachmentCount = 1;
        VkImageView view = m_fogOfwarTexture->GetImageView();
        framebufferInfo.pAttachments = &view;
        framebufferInfo.width = m_fogOfwarTexture->GetWidth();
        framebufferInfo.height = m_fogOfwarTexture->GetHeight();
        framebufferInfo.layers = 1;

        vkCreateFramebuffer(m_device, &framebufferInfo, nullptr, &m_visibilityFramebuffer);

        // Log these addresses
      
    }


    void VulkanFogOfWarPipelines::Destroy()
    {
        // 1. Destroy Pipelines
        if (m_pipeFogOverlay) vkDestroyPipeline(m_device, m_pipeFogOverlay, nullptr);
        if (m_pipeVisibilityMap) vkDestroyPipeline(m_device, m_pipeVisibilityMap, nullptr);
        if (m_pipeStencilWrite) vkDestroyPipeline(m_device, m_pipeStencilWrite, nullptr);

        // 2. Destroy Layouts
        if (m_pipelineLayout) vkDestroyPipelineLayout(m_device, m_pipelineLayout, nullptr);
        if (m_visibilityPipelineLayout) vkDestroyPipelineLayout(m_device, m_visibilityPipelineLayout, nullptr);
        if (m_descriptorSetLayout) vkDestroyDescriptorSetLayout(m_device, m_descriptorSetLayout, nullptr);

        // 3. Destroy Resources
        if (m_visibilityFramebuffer) vkDestroyFramebuffer(m_device, m_visibilityFramebuffer, nullptr);
        if (m_visibilityRenderPass) vkDestroyRenderPass(m_device, m_visibilityRenderPass, nullptr);
        if (m_descriptorPool) vkDestroyDescriptorPool(m_device, m_descriptorPool, nullptr);

        // 4. Clean up shader modules
        if (m_fs) vkDestroyShaderModule(m_device, m_fs, nullptr);
        if (m_vs) vkDestroyShaderModule(m_device, m_vs, nullptr);

        // Reset handles
        m_device = VK_NULL_HANDLE;
    }

}
