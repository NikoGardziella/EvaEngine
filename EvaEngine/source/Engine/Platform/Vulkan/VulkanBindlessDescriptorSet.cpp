#include "pch.h"
#include "VulkanBindlessDescriptorSet.h"
#include "VulkanContext.h"
#include "VulkanUtils.h"
#include "Engine/AssetManager/AssetManager.h"
#include "Engine/Scene/Component.h"
#include "Engine/Core/Core.h"
#include "Engine/Math/HashUtils.h"
#include "Engine/Platform/Vulkan/VulkanUtils.h"
#include <Engine.h>

namespace Engine {

    using std::uint32_t;

    VulkanBindlessDescriptorSetRenderer::VulkanBindlessDescriptorSetRenderer() {}
    VulkanBindlessDescriptorSetRenderer::VulkanBindlessDescriptorSetRenderer(VkDevice device, bool uab)
    {
        Init(device, uab);
    }
    VulkanBindlessDescriptorSetRenderer::~VulkanBindlessDescriptorSetRenderer() {}

    void VulkanBindlessDescriptorSetRenderer::Init(VkDevice device, bool updateAfterBindSupported)
    {
        VulkanContext* ctx = VulkanContext::Get();
        m_device = device;

        m_bindlessDescriptorsShader = std::make_shared<VulkanShader>(
            AssetManager::GetAssetPath("shaders/BindlessDesciptorset_shader.GLSL").string());

        CreateTileSampler(device);
        CreateBindlessSetLayout(device, updateAfterBindSupported);
        CreateBindlessPoolAndSet(device, updateAfterBindSupported);
        CreateTilesPipelineLayout(device);
        CreateTilesPipeline(device, ctx->GetPresentRenderPass());
        // Shader just for reference if you load it here
       
        // Instance buffers
        const uint32_t maxInstances = 4096; // tune as needed
        CreateInstanceBuffers(device, ctx->GetDeviceManager().GetPhysicalDevice(), m_instanceBuffer, maxInstances);

        EE_CORE_WARN("get this value from assset manager");
        m_atlasExtent = { 1024 , 1029, 1 }; // fallback; set via SetAtlasExtent() for correctness

        // Create the color array (one layer per resident tile)
        CreateColorArray(device, ctx->GetDeviceManager().GetPhysicalDevice());

        auto atlas = AssetManager::GetTileTextureIconAtlas();



        // Also tell the bindless system your per-tile pixel size (whatever you use for the per-tile layer)
        SetTileDimensions(TILE_PIXEL_WIDTH, TILE_PIXEL_HEIGHT);


    }

    void VulkanBindlessDescriptorSetRenderer::Shutdown(VkDevice device)
    {
        m_colorLayerPool.Shutdown(device);
        if (m_colorArrayImage)
        {
            vkDestroyImage(device, m_colorArrayImage, nullptr);
            m_colorArrayImage = VK_NULL_HANDLE;
        }
        if (m_colorArrayMem)
        {
            vkFreeMemory(device, m_colorArrayMem, nullptr);
            m_colorArrayMem = VK_NULL_HANDLE;
        }

        if (m_tileSampler)
        {
            vkDestroySampler(device, m_tileSampler, nullptr);
            m_tileSampler = VK_NULL_HANDLE;
        }
        if (m_tilesPipelineLayout)
        {
            vkDestroyPipelineLayout(device, m_tilesPipelineLayout, nullptr);
            m_tilesPipelineLayout = VK_NULL_HANDLE;
        }
        if (m_bindlessSetLayout)
        {
            vkDestroyDescriptorSetLayout(device, m_bindlessSetLayout, nullptr);
            m_bindlessSetLayout = VK_NULL_HANDLE;
        }
        if (m_descPool)
        {
            vkDestroyDescriptorPool(device, m_descPool, nullptr);
            m_descPool = VK_NULL_HANDLE;
        }
        for (int i = 0; i < 3; ++i)
        {
            if (m_instanceBuffer.mapped[i])
            {
                vkUnmapMemory(device, m_instanceBuffer.mem[i]);
                m_instanceBuffer.mapped[i] = nullptr;
            }
            if (m_instanceBuffer.buf[i]) vkDestroyBuffer(device, m_instanceBuffer.buf[i], nullptr);
            if (m_instanceBuffer.mem[i]) vkFreeMemory(device, m_instanceBuffer.mem[i], nullptr);
        }
    }

    

    void VulkanBindlessDescriptorSetRenderer::BeginFrame(uint32_t frameIndex, VkCommandBuffer uploadCB)
    {
        m_currentFrame = frameIndex;
        m_uploadCmdThisFrame = uploadCB;
        m_instances.clear();
        m_drawCount = 0;
    }

    void VulkanBindlessDescriptorSetRenderer::AddInstance(glm::vec2 worldPos, float zSortKey, uint32_t slot, uint32_t flags)
    {
        glm::vec2 size = glm::vec2(TILE_SIZE, TILE_SIZE * 2); // 1:2 ratio per tile
        RenderInstance I{ worldPos, size, zSortKey, slot, flags, 0 };
        m_instances.push_back(I);
    }

    void VulkanBindlessDescriptorSetRenderer::CreateTileSampler(VkDevice device)
    {
        VkSamplerCreateInfo sci{ VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO };
        sci.magFilter = VK_FILTER_NEAREST;
        sci.minFilter = VK_FILTER_NEAREST;
        sci.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
        sci.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        sci.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        sci.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        sci.maxAnisotropy = 1.0f;
        if (vkCreateSampler(device, &sci, nullptr, &m_tileSampler) != VK_SUCCESS)
        {
            EE_CORE_ERROR("Failed to create tile sampler");

        }
    }

    void VulkanBindlessDescriptorSetRenderer::CreateBindlessSetLayout(VkDevice device, bool updateAfterBindSupported)
    {
        VkDescriptorSetLayoutBinding bColor{};
        bColor.binding = 0;
        bColor.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        bColor.descriptorCount = MAX_RESIDENT;
        bColor.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;

        VkDescriptorSetLayoutBinding bStorage{};
        bStorage.binding = 1;
        bStorage.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
        bStorage.descriptorCount = MAX_RESIDENT;
        bStorage.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_COMPUTE_BIT;

        VkDescriptorSetLayoutBinding bInstances{};
        bInstances.binding = 2;
        bInstances.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        bInstances.descriptorCount = 1;
        bInstances.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_COMPUTE_BIT;

        VkDescriptorSetLayoutBinding bindings[3] = { bColor, bStorage, bInstances };

        VkDescriptorBindingFlags flags[3]{};
        flags[0] = VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT |
            (updateAfterBindSupported ? VK_DESCRIPTOR_BINDING_UPDATE_AFTER_BIND_BIT : 0);
        flags[1] = VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT |
            (updateAfterBindSupported ? VK_DESCRIPTOR_BINDING_UPDATE_AFTER_BIND_BIT : 0);
        flags[2] = 0;

        VkDescriptorSetLayoutBindingFlagsCreateInfo bindFlags{
            VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO
        };
        bindFlags.bindingCount = 3;
        bindFlags.pBindingFlags = flags;

        VkDescriptorSetLayoutCreateInfo dslci{ VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO };
        dslci.pNext = &bindFlags;
        dslci.bindingCount = 3;
        dslci.pBindings = bindings;
        dslci.flags = updateAfterBindSupported ? VK_DESCRIPTOR_SET_LAYOUT_CREATE_UPDATE_AFTER_BIND_POOL_BIT : 0;

        if (vkCreateDescriptorSetLayout(device, &dslci, nullptr, &m_bindlessSetLayout) != VK_SUCCESS)
        {
            EE_CORE_ERROR("Failed to create bindless set layout");

        }
    }


    void VulkanBindlessDescriptorSetRenderer::CreateTilesPipeline(VkDevice device, VkRenderPass renderPass)
    {
        
        VkPipelineShaderStageCreateInfo stages[2]{};
        stages[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        stages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
        stages[0].module = m_bindlessDescriptorsShader->GetVertexShaderModule();
        stages[0].pName = "main";

        stages[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        stages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
        stages[1].module = m_bindlessDescriptorsShader->GetFragmentShaderModule();
        stages[1].pName = "main";

        // --- 2) No vertex input (VS generates quad from gl_VertexIndex) ---
        VkPipelineVertexInputStateCreateInfo vi{ VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO };
        vi.vertexBindingDescriptionCount = 0;
        vi.pVertexBindingDescriptions = nullptr;
        vi.vertexAttributeDescriptionCount = 0;
        vi.pVertexAttributeDescriptions = nullptr;

        // --- 3) Triangle strip (4 verts -> 2 triangles) ---
        VkPipelineInputAssemblyStateCreateInfo ia{ VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO };
        ia.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP;
        ia.primitiveRestartEnable = VK_FALSE;

        // --- 4) Dynamic viewport/scissor ---
        VkPipelineViewportStateCreateInfo vp{ VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO };
        vp.viewportCount = 1;
        vp.scissorCount = 1;

        VkDynamicState dynStates[] = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
        VkPipelineDynamicStateCreateInfo dyn{ VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO };
        dyn.dynamicStateCount = (uint32_t)std::size(dynStates);
        dyn.pDynamicStates = dynStates;

        // --- 5) Rasterization ---
        VkPipelineRasterizationStateCreateInfo rs{ VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO };
        rs.polygonMode = VK_POLYGON_MODE_FILL;
        rs.cullMode = VK_CULL_MODE_NONE;       // tiles are sprites; disable cull
        rs.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
        rs.lineWidth = 1.0f;

        // --- 6) Multisample ---
        VkPipelineMultisampleStateCreateInfo ms{ VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO };
        ms.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

        // --- 7) Depth (optional) ---
        VkPipelineDepthStencilStateCreateInfo ds{ VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO };
        ds.depthTestEnable = VK_FALSE;
        ds.depthWriteEnable = VK_FALSE;

        // --- 8) Blending for alpha ---
        VkPipelineColorBlendAttachmentState blend{};
        blend.blendEnable = VK_TRUE;
        blend.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
        blend.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
        blend.colorBlendOp = VK_BLEND_OP_ADD;
        blend.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
        blend.dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
        blend.alphaBlendOp = VK_BLEND_OP_ADD;
        blend.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
            VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;

        VkPipelineColorBlendStateCreateInfo cb{ VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO };
        cb.attachmentCount = 1;
        cb.pAttachments = &blend;

        // --- 9) Create pipeline using your existing layout (set=0 + push constants) ---
        VkGraphicsPipelineCreateInfo pci{ VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO };
        pci.stageCount = 2;
        pci.pStages = stages;
        pci.pVertexInputState = &vi;
        pci.pInputAssemblyState = &ia;
        pci.pViewportState = &vp;
        pci.pRasterizationState = &rs;
        pci.pMultisampleState = &ms;
        pci.pDepthStencilState = &ds;
        pci.pColorBlendState = &cb;
        pci.pDynamicState = &dyn;
        pci.layout = m_tilesPipelineLayout; 
        pci.renderPass = renderPass;
        pci.subpass = 0;

        if (vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pci, nullptr, &m_tilesPipeline) != VK_SUCCESS)
        {
            EE_CORE_ERROR("Bindless: failed to create tiles pipeline");
        }

        vkDestroyShaderModule(device, m_bindlessDescriptorsShader->GetVertexShaderModule(), nullptr);
        vkDestroyShaderModule(device, m_bindlessDescriptorsShader->GetFragmentShaderModule(), nullptr);
    }


    void VulkanBindlessDescriptorSetRenderer::EndFrameAndUpload(uint32_t frameIndex)
    {
        // sort for painter’s order
        std::stable_sort(m_instances.begin(), m_instances.end(),
            [](auto& a, auto& b) { return a.zSortKey < b.zSortKey; });

        const size_t bytes = m_instances.size() * sizeof(RenderInstance);
        EE_CORE_ASSERT(bytes <= m_instanceBuffer.capacityBytes, "Instance buffer overflow");
        std::memcpy(m_instanceBuffer.mapped[frameIndex], m_instances.data(), bytes);
        m_drawCount = (uint32_t)m_instances.size();

        // descriptor binding=2 points to this frame’s SSBO
        WriteInstanceBufferToDescriptor(m_device, m_bindlessSet[frameIndex], m_instanceBuffer.buf[frameIndex]);
    }


    void VulkanBindlessDescriptorSetRenderer::CreateTilesPipelineLayout(VkDevice device)
    {
        VkPushConstantRange pc{};
        pc.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
        pc.offset = 0;
        pc.size = sizeof(glm::mat4);

        VkPipelineLayoutCreateInfo plci{ VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO };
        plci.setLayoutCount = 1;
        plci.pSetLayouts = &m_bindlessSetLayout;
        plci.pushConstantRangeCount = 1;
        plci.pPushConstantRanges = &pc;

        if (vkCreatePipelineLayout(device, &plci, nullptr, &m_tilesPipelineLayout) != VK_SUCCESS)
        {
            EE_CORE_ERROR("Failed to create pipeline layout for tiles");

        }
    }

    void VulkanBindlessDescriptorSetRenderer::CreateBindlessPoolAndSet(VkDevice device, bool updateAfterBindSupported)
    {
        VkDescriptorPoolSize poolSizes[] = {
            { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, MAX_RESIDENT },
            { VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,          MAX_RESIDENT },
            { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,         FRAMES_IN_FLIGHT }
        };

        VkDescriptorPoolCreateInfo dpci{ VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO };
        dpci.flags = updateAfterBindSupported ? VK_DESCRIPTOR_POOL_CREATE_UPDATE_AFTER_BIND_BIT : 0;
        dpci.maxSets = FRAMES_IN_FLIGHT;
        dpci.poolSizeCount = (uint32_t)std::size(poolSizes);
        dpci.pPoolSizes = poolSizes;

        if (vkCreateDescriptorPool(device, &dpci, nullptr, &m_descPool) != VK_SUCCESS)
        {
            EE_CORE_ERROR("Failed to create descriptor pool");
        }

        // Allocate one bindless set per frame-in-flight
        std::vector<VkDescriptorSetLayout> layouts(FRAMES_IN_FLIGHT, m_bindlessSetLayout);
        VkDescriptorSetAllocateInfo dsai{ VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO };
        dsai.descriptorPool = m_descPool;
        dsai.descriptorSetCount = FRAMES_IN_FLIGHT;
        dsai.pSetLayouts = layouts.data();

        if (vkAllocateDescriptorSets(device, &dsai, m_bindlessSet.data()) != VK_SUCCESS)
        {

            EE_CORE_ERROR("Failed to allocate bindless descriptor sets");
        }
    }

    void VulkanBindlessDescriptorSetRenderer::CreateInstanceBuffers(VkDevice dev, VkPhysicalDevice phys, InstanceBuffer& out, size_t maxInstances)
    {
        out.capacityBytes = maxInstances * sizeof(RenderInstance);
        for (int i = 0; i < 3; ++i)
        {
            VkBufferCreateInfo bi{ VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
            bi.size = out.capacityBytes;
            bi.usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
            bi.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
            if (vkCreateBuffer(dev, &bi, nullptr, &out.buf[i]) != VK_SUCCESS)
            {
                EE_CORE_ERROR("Failed to create instance buffer");
            }

            VkMemoryRequirements req{};
            vkGetBufferMemoryRequirements(dev, out.buf[i], &req);

            uint32_t typeIndex = VulkanContext::Get()->FindMemoryType(
                req.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
            );

            VkMemoryAllocateInfo ai{ VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO };
            ai.allocationSize = req.size;
            ai.memoryTypeIndex = typeIndex;
            vkAllocateMemory(dev, &ai, nullptr, &out.mem[i]);
            vkBindBufferMemory(dev, out.buf[i], out.mem[i], 0);
            vkMapMemory(dev, out.mem[i], 0, out.capacityBytes, 0, &out.mapped[i]);
        }
    }
    void VulkanBindlessDescriptorSetRenderer::CreateColorArray(VkDevice dev, VkPhysicalDevice phys)
    {
        const VkImageUsageFlags usage =
            VK_IMAGE_USAGE_SAMPLED_BIT |
            VK_IMAGE_USAGE_STORAGE_BIT |
            VK_IMAGE_USAGE_TRANSFER_DST_BIT;

        VkPhysicalDeviceProperties props{};
        vkGetPhysicalDeviceProperties(phys, &props);

        VkImageFormatProperties ifp{};
        VkResult fr = vkGetPhysicalDeviceImageFormatProperties(
            phys,
            VK_FORMAT_R8G8B8A8_UNORM,
            VK_IMAGE_TYPE_2D,
            VK_IMAGE_TILING_OPTIMAL,
            usage,
            0,
            &ifp
        );

        uint32_t byFormat = (fr == VK_SUCCESS) ? ifp.maxArrayLayers : props.limits.maxImageArrayLayers;
        m_arrayLayerCount = MAX_RESIDENT;
        if (m_arrayLayerCount > props.limits.maxImageArrayLayers)
        {
            m_arrayLayerCount = props.limits.maxImageArrayLayers;
        }
        if (m_arrayLayerCount > byFormat)
        {

            m_arrayLayerCount = byFormat;
        }

        // --- Create the 2D array image ---
        VkImageCreateInfo ci{ VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO };
        ci.flags = 0;
        ci.imageType = VK_IMAGE_TYPE_2D;
        ci.format = VK_FORMAT_R8G8B8A8_UNORM;
        ci.extent = { m_tileW, m_tileH, 1 };
        ci.mipLevels = 1;
        ci.arrayLayers = m_arrayLayerCount;   // actual clamped count
        ci.samples = VK_SAMPLE_COUNT_1_BIT;
        ci.tiling = VK_IMAGE_TILING_OPTIMAL;
        ci.usage = usage;
        ci.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        ci.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

        if (vkCreateImage(dev, &ci, nullptr, &m_colorArrayImage) != VK_SUCCESS)
        {
            EE_CORE_ERROR("CreateColorArray: vkCreateImage failed");
            return;
        }

        // --- Allocate + bind memory ---
        VkMemoryRequirements req{};
        vkGetImageMemoryRequirements(dev, m_colorArrayImage, &req);

        VkMemoryAllocateInfo ai{ VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO };
        ai.allocationSize = req.size;
        ai.memoryTypeIndex = VulkanContext::Get()->FindMemoryType(req.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

        if (vkAllocateMemory(dev, &ai, nullptr, &m_colorArrayMem) != VK_SUCCESS)
        {
            EE_CORE_ERROR("CreateColorArray: vkAllocateMemory failed");
            vkDestroyImage(dev, m_colorArrayImage, nullptr);
            m_colorArrayImage = VK_NULL_HANDLE;
            return;
        }

        if (vkBindImageMemory(dev, m_colorArrayImage, m_colorArrayMem, 0) != VK_SUCCESS)
        {
            EE_CORE_ERROR("CreateColorArray: vkBindImageMemory failed");
            vkFreeMemory(dev, m_colorArrayMem, nullptr);
            vkDestroyImage(dev, m_colorArrayImage, nullptr);
            m_colorArrayMem = VK_NULL_HANDLE;
            m_colorArrayImage = VK_NULL_HANDLE;
            return;
        }

        {
            VkCommandBuffer cb = VulkanContext::Get()->BeginSingleTimeCommands();

            VkImageMemoryBarrier b{ VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER };
            b.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
            b.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            b.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            b.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            b.image = m_colorArrayImage;
            b.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            b.subresourceRange.baseMipLevel = 0;
            b.subresourceRange.levelCount = 1;
            b.subresourceRange.baseArrayLayer = 0;
            b.subresourceRange.layerCount = m_arrayLayerCount;

            b.srcAccessMask = 0;
            b.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

            VkPipelineStageFlags srcStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
            VkPipelineStageFlags dstStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT | VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;

            vkCmdPipelineBarrier(cb,
                srcStage, dstStage,
                0,
                0, nullptr,
                0, nullptr,
                1, &b);

            VulkanContext::Get()->EndSingleTimeCommands(cb);
        }


        // --- Initialize  layer pool with the real layer count ---
        m_colorLayerPool.Init(dev, m_colorArrayImage, m_arrayLayerCount);
    }



    // ----- ColorLayerPool
    static VkImageView Make2DLayerView(VkDevice dev, VkImage img, VkFormat fmt, uint32_t layer)
    {
        VkImageViewCreateInfo vi{ VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO };
        vi.image = img;
        vi.viewType = VK_IMAGE_VIEW_TYPE_2D;
        vi.format = fmt;
        vi.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        vi.subresourceRange.baseMipLevel = 0;
        vi.subresourceRange.levelCount = 1;
        vi.subresourceRange.baseArrayLayer = layer;
        vi.subresourceRange.layerCount = 1;
        VkImageView v{};
        vkCreateImageView(dev, &vi, nullptr, &v);
        return v;
    }

    void VulkanBindlessDescriptorSetRenderer::ColorLayerPool::Init(VkDevice dev, VkImage img, uint32_t layerCount)
    {
        device = dev; image = img;
        views.resize(layerCount);
        freeList.reserve(layerCount);
        for (uint32_t i = 0; i < layerCount; ++i)
        {
            views[i] = Make2DLayerView(dev, img, format, i);
            freeList.push_back(i);
        }
    }

    void VulkanBindlessDescriptorSetRenderer::ColorLayerPool::Shutdown(VkDevice dev)
    {
        for (auto v : views) if (v) vkDestroyImageView(dev, v, nullptr);
        views.clear(); freeList.clear();
    }

    uint32_t VulkanBindlessDescriptorSetRenderer::ColorLayerPool::Acquire()
    {
        EE_CORE_ASSERT(!freeList.empty(), "ColorLayerPool exhausted");
        uint32_t i = freeList.back(); freeList.pop_back(); return i;
    }

    void VulkanBindlessDescriptorSetRenderer::ColorLayerPool::Release(uint32_t i)
    {
        freeList.push_back(i);
    }


    // ----- Descriptor writes
    void VulkanBindlessDescriptorSetRenderer::WriteCombinedImageSampler( VkDevice device, VkDescriptorSet set, uint32_t binding,
        uint32_t arrayIndex, VkSampler sampler, VkImageView view, VkImageLayout layout)
    {

        
        VkDescriptorImageInfo info{};
        info.sampler = sampler;
        info.imageView = view;
        info.imageLayout = layout;

        VkWriteDescriptorSet w{ VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET };
        w.dstSet = set;
        w.dstBinding = binding;
        w.dstArrayElement = arrayIndex;
        w.descriptorCount = 1;
        w.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        w.pImageInfo = &info;

        vkUpdateDescriptorSets(device, 1, &w, 0, nullptr);
    }
    void VulkanBindlessDescriptorSetRenderer::WriteStorageImage( VkDevice device, VkDescriptorSet set,
        uint32_t binding,  uint32_t arrayIndex, VkImageView view, VkImageLayout layout)
    {
        VkDescriptorImageInfo info{};
        info.imageView = view;
        info.imageLayout = layout;

        VkWriteDescriptorSet w{ VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET };
        w.dstSet = set;
        w.dstBinding = binding;
        w.dstArrayElement = arrayIndex;
        w.descriptorCount = 1;
        w.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
        w.pImageInfo = &info;

        vkUpdateDescriptorSets(device, 1, &w, 0, nullptr);
    }

    void VulkanBindlessDescriptorSetRenderer::WriteInstanceBufferToDescriptor(VkDevice dev, VkDescriptorSet set, VkBuffer buf)
    {
        VkDescriptorBufferInfo info{ buf, 0, VK_WHOLE_SIZE };
        VkWriteDescriptorSet w{ VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET };
        w.dstSet = set; w.dstBinding = 2; w.dstArrayElement = 0;
        w.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        w.descriptorCount = 1; w.pBufferInfo = &info;
        vkUpdateDescriptorSets(dev, 1, &w, 0, nullptr);
    }

    // ----- Layout transitions (basic, graphics queue)
    void VulkanBindlessDescriptorSetRenderer::TransitionImage(
        VkCommandBuffer cmd, VkImage img, VkImageLayout oldLayout, VkImageLayout newLayout,
        VkImageSubresourceRange range)
    {
        if (oldLayout == newLayout) return;
        VkImageMemoryBarrier b{ VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER };
        b.oldLayout = oldLayout;
        b.newLayout = newLayout;
        b.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        b.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        b.image = img;
        b.subresourceRange = range;

        // Simplified masks for common cases we use
        VkPipelineStageFlags srcStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        VkPipelineStageFlags dstStage = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;

        if (oldLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
            b.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
            b.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
            srcStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
            dstStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
        }
        else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
            b.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
            b.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
            srcStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
            dstStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
        }
        else if (oldLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL) {
            b.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
            b.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
            srcStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
            dstStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
        }
        else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
            b.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
            b.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
            srcStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
            dstStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
        }

        vkCmdPipelineBarrier(cmd, srcStage, dstStage, 0, 0, nullptr, 0, nullptr, 1, &b);
    }

    void VulkanBindlessDescriptorSetRenderer::TransitionImageLayer(
        VkCommandBuffer cmd, VkImage img, VkImageLayout oldLayout, VkImageLayout newLayout, uint32_t layer)
    {
        VkImageSubresourceRange r{};
        r.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        r.baseMipLevel = 0; r.levelCount = 1;
        r.baseArrayLayer = layer; r.layerCount = 1;
        TransitionImage(cmd, img, oldLayout, newLayout, r);
    }
    uint32_t VulkanBindlessDescriptorSetRenderer::EnsureTileResident(
        uint64_t uid, const glm::vec4& atlasUV, VkCommandBuffer uploadCB)
    {
        if (auto it = m_tileToSlot.find(uid); it != m_tileToSlot.end())
            return it->second;

        const uint32_t slot = m_colorLayerPool.Acquire();
        EE_CORE_ASSERT(slot < m_arrayLayerCount, "slot out of range");
        VkImageView view = m_colorLayerPool.View(slot);

        // Copy atlas patch -> array layer (outside render pass)
        CopyFromAtlasUVToLayer(uploadCB, AssetManager::GetTileTextureIconAtlas()->GetImage(),
            atlasUV, m_colorArrayImage, slot, m_tileW, m_tileH);

        // Write descriptors for *every* per-frame set (since UPDATE_AFTER_BIND is off)
        for (uint32_t f = 0; f < 3; ++f)
        {
            WriteCombinedImageSampler(m_device, m_bindlessSet[f], /*binding=*/0, /*arrayIndex=*/slot,
                m_tileSampler, view, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

            WriteStorageImage(m_device, m_bindlessSet[f], /*binding=*/1, /*arrayIndex=*/slot,
                view, VK_IMAGE_LAYOUT_GENERAL);
        }

        m_tileToSlot.emplace(uid, slot);
        return slot;
    }


    void VulkanBindlessDescriptorSetRenderer::CopyFromAtlasUVToLayer( VkCommandBuffer cmd, VkImage atlas, 
        const glm::vec4& uv, VkImage dstArray, uint32_t layer, uint32_t tileW, uint32_t tileH)
    {
        m_atlasExtent = AssetManager::GetTileTextureIconAtlas()->GetExtent();
        int32_t srcX = int32_t(std::round(uv.x * float(m_atlasExtent.width)));
        int32_t srcY = int32_t(std::round(uv.y * float(m_atlasExtent.height)));

        TransitionImage(cmd, atlas, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
            { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 });
        TransitionImageLayer(cmd, dstArray, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            layer);

        VkImageCopy region{};
        region.srcSubresource = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1 };
        region.dstSubresource = { VK_IMAGE_ASPECT_COLOR_BIT, 0, layer, 1 };
        region.srcOffset = { srcX, srcY, 0 };
        region.dstOffset = { 0, 0, 0 };
        region.extent = { tileW, tileH, 1 };

        vkCmdCopyImage(cmd, atlas, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
            dstArray, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

        TransitionImage(cmd, atlas, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
            { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 });
        TransitionImageLayer(cmd, dstArray, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
            layer);
    }

    bool VulkanBindlessDescriptorSetRenderer::IsInsideView(const Camera&, glm::vec2) const
    {
        // TODO: plug your real culling here
        return true;
    }


    void VulkanBindlessDescriptorSetRenderer::RecordTiles(VkCommandBuffer cmd,  uint32_t frameIndex, 
        const glm::mat4& VP, VkExtent2D fbExtent)
    {
        if (m_drawCount == 0) return;

        vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_tilesPipeline);

        VkDescriptorSet set0 = m_bindlessSet[frameIndex];
        vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_tilesPipelineLayout, /*firstSet*/0, 
            /*setCount*/1, &set0, 0, nullptr);

        vkCmdPushConstants(cmd, m_tilesPipelineLayout,
            VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
            0, sizeof(glm::mat4), &VP);

        VkViewport vp{ 0, 0, float(fbExtent.width), float(fbExtent.height), 0.f, 1.f };
        vkCmdSetViewport(cmd, 0, 1, &vp);
        VkRect2D sc{ {0,0}, fbExtent };
        vkCmdSetScissor(cmd, 0, 1, &sc);

        vkCmdDraw(cmd, /*vertexCount*/4, /*instanceCount*/m_drawCount, 0, 0);
    }


} 