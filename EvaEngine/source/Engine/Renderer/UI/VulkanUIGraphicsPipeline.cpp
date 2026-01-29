#include "pch.h"
#include "VulkanUIGraphicsPipeline.h"
#include "Engine/Platform/Vulkan/VulkanTexture.h"
#include "Engine/AssetManager/AssetManager.h"

#include <fstream>
#include <stdexcept>
#include <cstring>
#include <Engine/Platform/Vulkan/VulkanShader.h>
#include <Engine/Core/Assert.h>
#include <vulkan/vulkan_core.h>

namespace Engine {

   
    VulkanUIGraphicsPipeline::VulkanUIGraphicsPipeline(VkDevice device, VkPhysicalDevice physicalDevice)
        : m_device(device), m_physicalDevice(physicalDevice)
    {
        if (m_device == VK_NULL_HANDLE)
        {
            EE_CORE_ERROR("m_device is VK_NULL_HANDLE in VulkanUIGraphicsPipeline::Init!");
            throw std::runtime_error("Device not initialized");
        }
        EE_CORE_ASSERT(m_physicalDevice != VK_NULL_HANDLE, "VulkanUIGraphicsPipeline: physical device is null");
    }

    VulkanUIGraphicsPipeline::~VulkanUIGraphicsPipeline()
    {
        Shutdown();
    }

    void VulkanUIGraphicsPipeline::InitUIGraphicsPipeline(const UIInitConfig& cfg, uint32_t framesInFlight)
    {
        EE_CORE_ASSERT(cfg.renderPass != VK_NULL_HANDLE, "UI pipeline requires a valid render pass");
        EE_CORE_ASSERT(framesInFlight > 0, "framesInFlight must be > 0");
        EE_CORE_ASSERT(cfg.maxTextures > 0, "maxTextures must be > 0");


        m_cfg = cfg;
        m_framesInFlight = framesInFlight;

        CreateDescriptorSetLayouts();
        CreateDescriptorPool(framesInFlight);
        CreateCameraBuffers(framesInFlight);
        AllocateDescriptorSets(framesInFlight);

        m_uiShader  = std::make_shared<VulkanShader>(AssetManager::GetAssetPath("shaders/VulkanUIShader.GLSL").string());

        CreatePipelineLayout();
        CreatePipeline(cfg);
    }

    void VulkanUIGraphicsPipeline::Shutdown()
    {
        if (m_device == VK_NULL_HANDLE)
            return;

        vkDeviceWaitIdle(m_device);

        if (m_pipeline != VK_NULL_HANDLE)
        {
            vkDestroyPipeline(m_device, m_pipeline, nullptr);
            m_pipeline = VK_NULL_HANDLE;
        }

        if (m_pipelineLayout != VK_NULL_HANDLE)
        {
            vkDestroyPipelineLayout(m_device, m_pipelineLayout, nullptr);
            m_pipelineLayout = VK_NULL_HANDLE;
        }

        if (m_uiShader->GetVertexShaderModule() != VK_NULL_HANDLE)
        {
            vkDestroyShaderModule(m_device, m_uiShader->GetVertexShaderModule(), nullptr);
            
        }

        if (m_uiShader->GetFragmentShaderModule() != VK_NULL_HANDLE)
        {
            vkDestroyShaderModule(m_device, m_uiShader->GetFragmentShaderModule(), nullptr);
        }

        DestroyCameraBuffers();

        if (m_descriptorPool != VK_NULL_HANDLE)
        {
            vkDestroyDescriptorPool(m_device, m_descriptorPool, nullptr);
            m_descriptorPool = VK_NULL_HANDLE;
        }

        if (m_setLayoutCamera != VK_NULL_HANDLE)
        {
            vkDestroyDescriptorSetLayout(m_device, m_setLayoutCamera, nullptr);
            m_setLayoutCamera = VK_NULL_HANDLE;
        }

        if (m_setLayoutUITextures != VK_NULL_HANDLE)
        {
            vkDestroyDescriptorSetLayout(m_device, m_setLayoutUITextures, nullptr);
            m_setLayoutUITextures = VK_NULL_HANDLE;
        }

        m_setCamera.clear();
        m_setUITextures.clear();
        m_cameraBuffers.clear();
        m_framesInFlight = 0;
        m_cfg = {};
    }


    VkDescriptorSet VulkanUIGraphicsPipeline::GetCameraDescriptorSet(uint32_t frame) const
    {
        EE_CORE_ASSERT(frame < m_setCamera.size(), "GetCameraDescriptorSet: frame out of range");
        return m_setCamera[frame];
    }

    VkDescriptorSet VulkanUIGraphicsPipeline::GetUITextureDescriptorSet(uint32_t frame) const
    {
        EE_CORE_ASSERT(frame < m_setUITextures.size(), "GetUITextureDescriptorSet: frame out of range");
        return m_setUITextures[frame];
    }

    void VulkanUIGraphicsPipeline::UpdateCameraUBO(uint32_t frame, const CameraUBO& data)
    {

      
        EE_CORE_ASSERT(frame < m_cameraBuffers.size(), "UpdateCameraUBO: frame out of range");
        EE_CORE_ASSERT(m_cameraBuffers[frame].mapped != nullptr, "UpdateCameraUBO: camera buffer not mapped");

        std::memcpy(m_cameraBuffers[frame].mapped, &data, sizeof(CameraUBO));
    }

    void VulkanUIGraphicsPipeline::UpdateUITextures(uint32_t frame, const std::vector<Ref<VulkanTexture>>& textures)
    {
        EE_CORE_ASSERT(frame < m_setUITextures.size(), "UpdateUITextures: frame out of range");
        EE_CORE_ASSERT(m_setUITextures[frame] != VK_NULL_HANDLE, "UpdateUITextures: descriptor set is null");
        EE_CORE_ASSERT(m_cfg.fallbackTexture != nullptr, "UpdateUITextures: fallbackTexture is null");
        EE_CORE_ASSERT((uint32_t)textures.size() <= m_cfg.maxTextures, "UpdateUITextures: textures.size exceeds maxTextures");

        std::vector<VkDescriptorImageInfo> infos(m_cfg.maxTextures);

        for (uint32_t i = 0; i < m_cfg.maxTextures; ++i)
        {
            Ref<VulkanTexture> t = m_cfg.fallbackTexture;
            if (i < (uint32_t)textures.size() && textures[i] != nullptr)
                t = textures[i];

            infos[i].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            infos[i].imageView = t->GetImageView();
            infos[i].sampler = t->GetSampler();
        }

        VkWriteDescriptorSet write{ VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET };
        write.dstSet = m_setUITextures[frame];
        write.dstBinding = 0;
        write.dstArrayElement = 0;
        write.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        write.descriptorCount = (uint32_t)infos.size();
        write.pImageInfo = infos.data();

        vkUpdateDescriptorSets(m_device, 1, &write, 0, nullptr);
    }


    void VulkanUIGraphicsPipeline::CreateDescriptorSetLayouts()
    {
        // Verify device first
        if (m_device == VK_NULL_HANDLE) {
            EE_CORE_ERROR("m_device is null in CreateDescriptorSetLayouts!");
            throw std::runtime_error("Device not initialized");
        }

        // set 0: camera UBO (binding 0)
        VkDescriptorSetLayoutBinding camUBO{};
        camUBO.binding = 0;
        camUBO.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        camUBO.descriptorCount = 1;
        camUBO.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
        camUBO.pImmutableSamplers = nullptr;

        VkDescriptorSetLayoutCreateInfo camLayoutCI{};
        camLayoutCI.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        camLayoutCI.bindingCount = 1;
        camLayoutCI.pBindings = &camUBO;

        VkResult result = vkCreateDescriptorSetLayout(m_device, &camLayoutCI, nullptr, &m_setLayoutCamera);
        if (result != VK_SUCCESS || m_setLayoutCamera == VK_NULL_HANDLE) {
            EE_CORE_ERROR("Failed to create UI camera descriptor set layout: {}", (int)result);
            throw std::runtime_error("Failed to create camera descriptor set layout");
        }

        // set 1: texture array
        VkDescriptorSetLayoutBinding texArr{};
        texArr.binding = 0;
        texArr.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        texArr.descriptorCount = m_cfg.maxTextures;
        texArr.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
        texArr.pImmutableSamplers = nullptr;

        VkDescriptorSetLayoutCreateInfo texLayoutCI{};
        texLayoutCI.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        texLayoutCI.bindingCount = 1;
        texLayoutCI.pBindings = &texArr;

        result = vkCreateDescriptorSetLayout(m_device, &texLayoutCI, nullptr, &m_setLayoutUITextures);
        if (result != VK_SUCCESS || m_setLayoutUITextures == VK_NULL_HANDLE) {
            EE_CORE_ERROR("Failed to create UI textures descriptor set layout: {}", (int)result);
            throw std::runtime_error("Failed to create textures descriptor set layout");
        }

        EE_CORE_INFO("UI m_setLayoutCamera = {}", (void*)m_setLayoutCamera);
        EE_CORE_INFO("UI m_setLayoutUITextures = {}", (void*)m_setLayoutUITextures);
    }

    void VulkanUIGraphicsPipeline::CreateDescriptorPool(uint32_t framesInFlight)
    {
        if (m_device == VK_NULL_HANDLE) {
            EE_CORE_ERROR("m_device is null in CreateDescriptorPool!");
            throw std::runtime_error("Device not initialized");
        }

        std::array<VkDescriptorPoolSize, 2> sizes{};
        sizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        sizes[0].descriptorCount = framesInFlight;
        sizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        sizes[1].descriptorCount = framesInFlight * m_cfg.maxTextures;

        VkDescriptorPoolCreateInfo poolCI{};
        poolCI.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        poolCI.poolSizeCount = (uint32_t)sizes.size();
        poolCI.pPoolSizes = sizes.data();
        poolCI.maxSets = framesInFlight * 2;

        VkResult result = vkCreateDescriptorPool(m_device, &poolCI, nullptr, &m_descriptorPool);
        if (result != VK_SUCCESS) {
            EE_CORE_ERROR("vkCreateDescriptorPool failed: {}", (int)result);
            throw std::runtime_error("Failed to create descriptor pool");
        }

        if (m_descriptorPool == VK_NULL_HANDLE) {
            EE_CORE_ERROR("Descriptor pool is null after creation!");
            throw std::runtime_error("Descriptor pool is null");
        }
    }

    void VulkanUIGraphicsPipeline::AllocateDescriptorSets(uint32_t framesInFlight)
    {

        EE_CORE_INFO("UI VkDevice        = {}", (void*)m_device);
        EE_CORE_INFO("UI VkPhysicalDevice= {}", (void*)m_physicalDevice);
        EE_CORE_INFO("UI VkDescriptorPool= {}", (void*)m_descriptorPool);
        EE_CORE_INFO("UI SetLayout       = {}", (void*)m_setLayoutCamera);

        m_setCamera.resize(framesInFlight);
        m_setUITextures.resize(framesInFlight);

        // Allocate camera sets
        {
            std::vector<VkDescriptorSetLayout> layouts(framesInFlight, m_setLayoutCamera);
            VkDescriptorSetAllocateInfo alloc{};
            alloc.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
            alloc.descriptorPool = m_descriptorPool;
            alloc.descriptorSetCount = framesInFlight;
            alloc.pSetLayouts = layouts.data();

            VkResult result = vkAllocateDescriptorSets(m_device, &alloc, m_setCamera.data());
            if (result != VK_SUCCESS) {
                EE_CORE_ERROR("Failed to allocate UI camera descriptor sets: {}", (int)result);
                throw std::runtime_error("Failed to allocate UI camera descriptor sets");
            }

            // Verify they're not null
            for (uint32_t i = 0; i < framesInFlight; ++i) {
                if (m_setCamera[i] == VK_NULL_HANDLE) {
                    EE_CORE_ERROR("Camera descriptor set {} is VK_NULL_HANDLE after allocation!", i);
                    throw std::runtime_error("Camera descriptor set is null");
                }
            }
        }

        // Allocate texture sets
        {
            std::vector<VkDescriptorSetLayout> layouts(framesInFlight, m_setLayoutUITextures);
            VkDescriptorSetAllocateInfo alloc{};
            alloc.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
            alloc.descriptorPool = m_descriptorPool;
            alloc.descriptorSetCount = framesInFlight;
            alloc.pSetLayouts = layouts.data();

            VkResult result = vkAllocateDescriptorSets(m_device, &alloc, m_setUITextures.data());
            if (result != VK_SUCCESS) {
                EE_CORE_ERROR("Failed to allocate UI texture descriptor sets: {}", (int)result);
                throw std::runtime_error("Failed to allocate UI texture descriptor sets");
            }
        }

        // Update camera descriptor sets
        for (uint32_t frame = 0; frame < framesInFlight; ++frame)
        {
            VkDescriptorBufferInfo bi{};
            bi.buffer = m_cameraBuffers[frame].buffer;
            bi.offset = 0;
            bi.range = sizeof(CameraUBO);

            VkWriteDescriptorSet write{};
            write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            write.dstSet = m_setCamera[frame];
            write.dstBinding = 0;
            write.dstArrayElement = 0;
            write.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
            write.descriptorCount = 1;
            write.pBufferInfo = &bi;

            vkUpdateDescriptorSets(m_device, 1, &write, 0, nullptr);
        }
    }

    void VulkanUIGraphicsPipeline::CreateCameraBuffers(uint32_t framesInFlight)
    {
        m_cameraBuffers.resize(framesInFlight);
        if (m_device == VK_NULL_HANDLE)
        {
            EE_CORE_ERROR("m_device is null in CreateCameraBuffers!");
            return;
        }
        EE_CORE_INFO("UI pipeline device = {}", (void*)m_device);
        EE_CORE_INFO("UI pipeline device = {}", (void*)m_physicalDevice);
        for (uint32_t i = 0; i < framesInFlight; ++i)
        {
            CameraBuffer& cb = m_cameraBuffers[i];
            cb.size = sizeof(CameraUBO);

            BufferUtils::CreateBufferMapped(cb.size,
                VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                cb.buffer, cb.memory, &cb.mapped);

            EE_CORE_ASSERT(cb.mapped != nullptr, "Failed to map UI camera buffer");
            std::memset(cb.mapped, 0, (size_t)cb.size);
        }
    }

    void VulkanUIGraphicsPipeline::DestroyCameraBuffers()
    {
        for (auto& cb : m_cameraBuffers)
        {
            DestroyBuffer(cb.buffer, cb.memory, &cb.mapped);
            cb.size = 0;
        }
    }

    void VulkanUIGraphicsPipeline::CreatePipelineLayout()
    {
        std::array<VkDescriptorSetLayout, 2> setLayouts = { m_setLayoutCamera, m_setLayoutUITextures };

        VkPushConstantRange pcr{};
        pcr.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
        pcr.offset = 0;
        pcr.size = sizeof(UIPushConstants);

        VkPipelineLayoutCreateInfo pl{};
        pl.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        pl.setLayoutCount = (uint32_t)setLayouts.size();
        pl.pSetLayouts = setLayouts.data();
        pl.pushConstantRangeCount = 1;
        pl.pPushConstantRanges = &pcr;

        VkResult res = vkCreatePipelineLayout(m_device, &pl, nullptr, &m_pipelineLayout);
        EE_CORE_INFO("vkCreatePipelineLayout res={} layout={}", (int)res, (void*)m_pipelineLayout);

        if (res != VK_SUCCESS || m_pipelineLayout == VK_NULL_HANDLE)
        {
            EE_CORE_ERROR("CreatePipelineLayout failed");
            return;
        }
    }


    void VulkanUIGraphicsPipeline::FillVertexInputState(VkPipelineVertexInputStateCreateInfo& vi,
        std::array<VkVertexInputBindingDescription, 1>& bindings,
        std::array<VkVertexInputAttributeDescription, 4>& attrs) const
    {
        // VulkanQuadVertex:
        // glm::vec3 Position;  (location 0) format R32G32B32_SFLOAT
        // glm::vec4 Color;     (location 1) format R32G32B32A32_SFLOAT
        // glm::vec2 TexCoord;  (location 2) format R32G32_SFLOAT
        // uint32_t  TexIndex;  (location 3) format R32_UINT

        bindings[0].binding = 0;
        bindings[0].stride = sizeof(VulkanUIQuadVertex);
        bindings[0].inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

        attrs[0].location = 0;
        attrs[0].binding = 0;
        attrs[0].format = VK_FORMAT_R32G32B32_SFLOAT;
        attrs[0].offset = offsetof(VulkanUIQuadVertex, Position);


        attrs[1].location = 1;
        attrs[1].binding = 0;
        attrs[1].format = VK_FORMAT_R32G32B32A32_SFLOAT;
        attrs[1].offset = offsetof(VulkanUIQuadVertex, Color);

        attrs[2].location = 2;
        attrs[2].binding = 0;
        attrs[2].format = VK_FORMAT_R32G32_SFLOAT;
        attrs[2].offset = offsetof(VulkanUIQuadVertex, TexCoord);


        attrs[3].location = 3;
        attrs[3].binding = 0;
        attrs[3].format = VK_FORMAT_R32_UINT;
        attrs[3].offset = offsetof(VulkanUIQuadVertex, TexIndex);


        vi = {};
        vi.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
        vi.vertexBindingDescriptionCount = (uint32_t)bindings.size();
        vi.pVertexBindingDescriptions = bindings.data();
        vi.vertexAttributeDescriptionCount = (uint32_t)attrs.size();
        vi.pVertexAttributeDescriptions = attrs.data();
    }

    void VulkanUIGraphicsPipeline::CreatePipeline(const UIInitConfig& cfg)
    {
        EE_CORE_ASSERT(m_pipelineLayout != VK_NULL_HANDLE, "UI pipeline layout not created");

        VkPipelineShaderStageCreateInfo stages[2]{};
        stages[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        stages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
        stages[0].module = m_uiShader->GetVertexShaderModule();
        stages[0].pName = "main";

        stages[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        stages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
        stages[1].module = m_uiShader->GetFragmentShaderModule();
        stages[1].pName = "main";

        // Vertex input
        VkPipelineVertexInputStateCreateInfo vi{};
        std::array<VkVertexInputBindingDescription, 1> bindings{};
        std::array<VkVertexInputAttributeDescription, 4> attrs{};
        FillVertexInputState(vi, bindings, attrs);

        VkPipelineInputAssemblyStateCreateInfo ia{};
        ia.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
        ia.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        ia.primitiveRestartEnable = VK_FALSE;

        // Dynamic viewport/scissor (UI often uses scissor later)
        VkPipelineViewportStateCreateInfo vp{};
        vp.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
        vp.viewportCount = 1;
        vp.scissorCount = 1;

        std::array<VkDynamicState, 2> dynStates = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
        VkPipelineDynamicStateCreateInfo dyn{};
        dyn.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
        dyn.dynamicStateCount = (uint32_t)dynStates.size();
        dyn.pDynamicStates = dynStates.data();

        VkPipelineRasterizationStateCreateInfo rs{};
        rs.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
        rs.depthClampEnable = VK_FALSE;
        rs.rasterizerDiscardEnable = VK_FALSE;
        rs.polygonMode = VK_POLYGON_MODE_FILL;
        rs.cullMode = VK_CULL_MODE_NONE; // UI: easiest
        rs.frontFace = VK_FRONT_FACE_CLOCKWISE;
        rs.depthBiasEnable = VK_FALSE;
        rs.lineWidth = 1.0f;

        VkPipelineMultisampleStateCreateInfo ms{};
        ms.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
        ms.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
        ms.sampleShadingEnable = VK_FALSE;

        // UI: depth test off
        VkPipelineDepthStencilStateCreateInfo ds{};
        ds.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
        ds.depthTestEnable = VK_FALSE;
        ds.depthWriteEnable = VK_FALSE;
        ds.depthCompareOp = VK_COMPARE_OP_ALWAYS;
        ds.depthBoundsTestEnable = VK_FALSE;
        ds.stencilTestEnable = VK_FALSE;

        // UI: alpha blending on
        VkPipelineColorBlendAttachmentState blendAtt{};
        blendAtt.blendEnable = VK_TRUE;
        blendAtt.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
        blendAtt.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
        blendAtt.colorBlendOp = VK_BLEND_OP_ADD;
        blendAtt.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
        blendAtt.dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
        blendAtt.alphaBlendOp = VK_BLEND_OP_ADD;
        blendAtt.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
            VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;

        VkPipelineColorBlendStateCreateInfo cb{};
        cb.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
        cb.logicOpEnable = VK_FALSE;
        cb.attachmentCount = 1;
        cb.pAttachments = &blendAtt;

        VkGraphicsPipelineCreateInfo gp{};
        gp.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
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
        gp.layout = m_pipelineLayout;
        gp.renderPass = cfg.renderPass;
        gp.subpass = cfg.subpassIndex;

   
        vkCreateGraphicsPipelines(m_device, VK_NULL_HANDLE, 1, &gp, nullptr, &m_pipeline);
    }

 
    uint32_t VulkanUIGraphicsPipeline::FindMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties) const
    {
        VkPhysicalDeviceMemoryProperties memProps{};
        vkGetPhysicalDeviceMemoryProperties(m_physicalDevice, &memProps);

        for (uint32_t i = 0; i < memProps.memoryTypeCount; ++i)
        {
            if ((typeFilter & (1u << i)) && (memProps.memoryTypes[i].propertyFlags & properties) == properties)
                return i;
        }

        EE_CORE_ASSERT(false, "Failed to find suitable memory type");
        return 0;
    }

   

    void VulkanUIGraphicsPipeline::DestroyBuffer(VkBuffer& buffer, VkDeviceMemory& memory, void** mapped)
    {
        if (mapped && *mapped)
        {
            vkUnmapMemory(m_device, memory);
            *mapped = nullptr;
        }

        if (buffer != VK_NULL_HANDLE)
        {
            vkDestroyBuffer(m_device, buffer, nullptr);
            buffer = VK_NULL_HANDLE;
        }

        if (memory != VK_NULL_HANDLE)
        {
            vkFreeMemory(m_device, memory, nullptr);
            memory = VK_NULL_HANDLE;
        }
    }

}
