#include "pch.h"
#include "VulkanBindlessDescriptorSet.h"
#include "VulkanContext.h"
#include "VulkanUtils.h"
#include "Engine/AssetManager/AssetManager.h"
#include "Engine/Scene/Component.h"

#include "Engine/Math/HashUtils.h"
#include "Engine/Platform/Vulkan/VulkanUtils.h"
#include <Engine.h>
#include <Engine/Renderer/Renderer2D/VulkanRenderer2D.h>
#include <Engine/Map/TextureStreaming/TextureStreamingSystem.h>
#include <Engine/AssetManager/Utils/Statistics.h>
#include <Engine/Core/Assert.h>

#include "Engine/Renderer/Lights/VulkanLighting.h"
#include <Engine/Renderer/Lights/GPULightBuffer.h>

namespace Engine {

    using std::uint32_t;

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

        m_computeShader = std::make_shared<VulkanShader>(AssetManager::GetAssetPath("shaders/compute.comp").string());
        m_effectShader = std::make_shared<VulkanShader>(AssetManager::GetAssetPath("shaders/Effect_shader.comp").string());


        CreateTileSampler(device);
        CreateBindlessSetLayout(device, updateAfterBindSupported);
        CreateComputeArrayDescriptorSetLayout(MAX_RESIDENT_LAYERS, false);
        CreateEffectsDescriptorSetLayout();

        CreateComputeDescriptorSet(ctx->GetComputeDescriptorPool());
        CreateEffectsDescriptorSets(ctx->GetEffectDescriptorPool());
        CreateBindlessPoolAndSet(device, updateAfterBindSupported);
        CreateEffectsPipelineLayout();
        CreateTilesPipelineLayout(device);
        CreateComputeGraphicsPipeline();
        CreateEffectsPipeline();

        CreateTilesPipeline(device, ctx->GetGameRenderPass());
       
        // Instance buffers
        const uint32_t maxInstances = MAX_RESIDENT_LAYERS;
        CreateInstanceBuffers(device, ctx->GetDeviceManager().GetPhysicalDevice(), m_instanceBuffer, maxInstances);

        EE_CORE_WARN("get this value from assset manager");
        m_atlasExtent = { 0 , 0, 1 }; // fallback;

        // Create the color array (one layer per resident tile)
        CreateColorArray(device, ctx->GetDeviceManager().GetPhysicalDevice());
        CreatePropsArray(device, ctx->GetDeviceManager().GetPhysicalDevice());

        auto atlas = AssetManager::GetTileTextureIconAtlas();

        SetTileDimensions(TILE_PIXEL_WIDTH, TILE_PIXEL_HEIGHT);

    }

    void VulkanBindlessDescriptorSetRenderer::Shutdown(VkDevice device)
    {
        m_colorLayerPool.Destroy();
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
        for (int i = 0; i < FRAMES_IN_FLIGHT; ++i)
        {
            if (m_instanceBuffer.mapped[i])
            {
                vkUnmapMemory(device, m_instanceBuffer.mem[i]);
                m_instanceBuffer.mapped[i] = nullptr;
            }
            if (m_instanceBuffer.buf[i]) vkDestroyBuffer(device, m_instanceBuffer.buf[i], nullptr);
            if (m_instanceBuffer.mem[i]) vkFreeMemory(device, m_instanceBuffer.mem[i], nullptr);

            

        }
        GPUStats& stats = GPUStats::Get();
        stats.RemoveBuffer(m_instanceBuffer.capacityBytes);

        vkDestroyPipeline(m_device, m_computePipeline, nullptr);
        vkDestroyPipelineLayout(m_device, m_computePipelineLayout, nullptr);

    }

    

    void VulkanBindlessDescriptorSetRenderer::BeginFrame(uint32_t frameIndex)
    {
        m_currentFrame = frameIndex;
        m_instances.clear();
        m_drawCount = 0;
    }

    void VulkanBindlessDescriptorSetRenderer::AddSpriteInstance(glm::vec2 worldCenter, float zKey, uint32_t spriteSlot,
        glm::uvec2 uvMin16, glm::uvec2 uvMax16, glm::vec2 sizeWorld, float rotation)
    {
        RenderInstance I{};
        I.worldPos = worldCenter;          // using the “center” convention you fixed
        I.size = sizeWorld;
        I.zSortKey = zKey;
        I.slot = spriteSlot;
        I.flags = 1u;                   // bit0 => use binding 3 (uSprites[])
        I._pad0 = 0;
        I.uvMin16 = uvMin16;
        I.uvMax16 = uvMax16;
        I.rotation = rotation;
        m_instances.push_back(I);
    }


    void VulkanBindlessDescriptorSetRenderer::AddInstance(glm::vec2 worldPos, float zSortKey, uint32_t slot, float rotation, uint32_t flags)
    {
        glm::vec2 size = glm::vec2(TILE_SIZE, TILE_SIZE * 2); // 1:2 ratio per tile

        RenderInstance I{};
        I.worldPos = worldPos;   // center point (as before)
        I.size = size;
        I.zSortKey = zSortKey;
        I.slot = slot;
        I.flags = (flags & ~1u);              // ensure isSprite = 0 for tiles
        I._pad0 = 0;
        I.uvMin16 = { 0u, 0u };                 // full texture UVs
        I.uvMax16 = { 65535u, 65535u };
        I.rotation = rotation;
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
        // binding 0: tiles sampled array (you already have this)
        VkDescriptorSetLayoutBinding bColor{};
        bColor.binding = 0;
        bColor.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        bColor.descriptorCount = MAX_RESIDENT;
        bColor.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;

        // binding 1: storage images (compute writes) — keep as-is
        VkDescriptorSetLayoutBinding bStorage{};
        bStorage.binding = 1;
        bStorage.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
        bStorage.descriptorCount = MAX_RESIDENT;
        bStorage.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_COMPUTE_BIT;

        // binding 2: instances SSBO — keep as-is
        VkDescriptorSetLayoutBinding bInstances{};
        bInstances.binding = 2;
        bInstances.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        bInstances.descriptorCount = 1;
        bInstances.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_COMPUTE_BIT;

        // binding 3: spritesheets sampled array
        VkDescriptorSetLayoutBinding bSprites{};
        bSprites.binding = 3;
        bSprites.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        bSprites.descriptorCount = MAX_SPRITESHEETS;
        bSprites.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

        // binding 4: light buffer (uniform buffer)
        VkDescriptorSetLayoutBinding bLights{};
        bLights.binding = 4;
        bLights.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        bLights.descriptorCount = 1;
        bLights.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

        VkDescriptorSetLayoutBinding bindings[5] = { bColor, bStorage, bInstances, bSprites, bLights };

        VkDescriptorBindingFlags flags[5]{};
        flags[0] = VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT |
            (updateAfterBindSupported ? VK_DESCRIPTOR_BINDING_UPDATE_AFTER_BIND_BIT : 0);
        flags[1] = VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT |
            (updateAfterBindSupported ? VK_DESCRIPTOR_BINDING_UPDATE_AFTER_BIND_BIT : 0);
        flags[2] = 0;
        flags[3] = VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT |
            (updateAfterBindSupported ? VK_DESCRIPTOR_BINDING_UPDATE_AFTER_BIND_BIT : 0);
        flags[4] = 0;  // Light buffer doesn't need partial bound or update after bind

        VkDescriptorSetLayoutBindingFlagsCreateInfo bindFlags{
            VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO
        };
        bindFlags.bindingCount = 5;
        bindFlags.pBindingFlags = flags;

        VkDescriptorSetLayoutCreateInfo dslci{ VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO };
        dslci.pNext = &bindFlags;
        dslci.bindingCount = 5;
        dslci.pBindings = bindings;
        dslci.flags = updateAfterBindSupported ? VK_DESCRIPTOR_SET_LAYOUT_CREATE_UPDATE_AFTER_BIND_POOL_BIT : 0;

        if (vkCreateDescriptorSetLayout(device, &dslci, nullptr, &m_bindlessSetLayout) != VK_SUCCESS)
        {
            EE_CORE_ASSERT(false, "failed to create bindless set layout");
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
        rs.cullMode = VK_CULL_MODE_BACK_BIT;       // tiles are sprites; disable cull
        rs.frontFace = VK_FRONT_FACE_CLOCKWISE;
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


        VkPipelineDepthStencilStateCreateInfo depthStencil{};
        depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
        depthStencil.depthTestEnable = VK_FALSE;
        depthStencil.depthWriteEnable = VK_FALSE;
        depthStencil.depthCompareOp = VK_COMPARE_OP_ALWAYS;
        depthStencil.depthBoundsTestEnable = VK_FALSE;
        depthStencil.stencilTestEnable = VK_FALSE;



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
        pci.pDepthStencilState = &depthStencil;

        if (vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pci, nullptr, &m_tilesPipeline) != VK_SUCCESS)
        {
            EE_CORE_ERROR("Bindless: failed to create tiles pipeline");
        }

        vkDestroyShaderModule(device, m_bindlessDescriptorsShader->GetVertexShaderModule(), nullptr);
        vkDestroyShaderModule(device, m_bindlessDescriptorsShader->GetFragmentShaderModule(), nullptr);
    }


    void VulkanBindlessDescriptorSetRenderer::EndFrameAndUpload(uint32_t frameIndex)
    {
        // sort for draw top to down.
        std::stable_sort(m_instances.begin(), m_instances.end(),
            [](auto& a, auto& b)
            {
                return a.zSortKey > b.zSortKey;
            });

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
            // Binding 0 (tiles) + Binding 3 (spritesheets) = combined samplers
            { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, MAX_RESIDENT + MAX_SPRITESHEETS },

            // Binding 1: storage images
            { VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, MAX_RESIDENT },

            // Binding 2: instances SSBO (1 buffer)
            { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1 },

            // Binding 4: light buffer (1 uniform buffer)
            { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1 }
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
        for (int i = 0; i < FRAMES_IN_FLIGHT; ++i)
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

            GPUStats& stats = GPUStats::Get();
            stats.AddBuffer(req.size);

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
        m_colorLayerPool.Init(dev, m_colorArrayImage, VK_FORMAT_R8G8B8A8_UNORM, m_arrayLayerCount);
    }


    void VulkanBindlessDescriptorSetRenderer::CreatePropsArray(VkDevice dev, VkPhysicalDevice phys)
    {
        VkImageCreateInfo ci{ VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO };
        ci.imageType = VK_IMAGE_TYPE_2D;
        ci.format = VK_FORMAT_R8G8B8A8_UINT;
        ci.extent = { m_tileW, m_tileH, 1 };
        ci.mipLevels = 1;
        ci.arrayLayers = m_arrayLayerCount;
        ci.samples = VK_SAMPLE_COUNT_1_BIT;
        ci.tiling = VK_IMAGE_TILING_OPTIMAL;
        ci.usage = VK_IMAGE_USAGE_STORAGE_BIT |
            VK_IMAGE_USAGE_TRANSFER_DST_BIT |
            VK_IMAGE_USAGE_SAMPLED_BIT; // for effects
        ci.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

        vkCreateImage(dev, &ci, nullptr, &m_propsArrayImage);

        VkMemoryRequirements req{};
        vkGetImageMemoryRequirements(dev, m_propsArrayImage, &req);

        uint32_t typeIndex = VulkanContext::Get()->FindMemoryType(req.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
        VkMemoryAllocateInfo ai{ VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO };
        ai.allocationSize = req.size;
        ai.memoryTypeIndex = typeIndex;
        vkAllocateMemory(dev, &ai, nullptr, &m_propsArrayMem);
        vkBindImageMemory(dev, m_propsArrayImage, m_propsArrayMem, 0);

        // One-time transition: ALL layers UNDEFINED -> GENERAL
        VkCommandBuffer cmd = VulkanContext::Get()->BeginSingleTimeCommands();

        VkImageMemoryBarrier barrier{ VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER };
        barrier.srcAccessMask = 0;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT;
        barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        barrier.newLayout = VK_IMAGE_LAYOUT_GENERAL;
        barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.image = m_propsArrayImage;
        barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        barrier.subresourceRange.baseMipLevel = 0;
        barrier.subresourceRange.levelCount = 1;
        barrier.subresourceRange.baseArrayLayer = 0;
        barrier.subresourceRange.layerCount = m_arrayLayerCount;

        vkCmdPipelineBarrier(cmd,
            VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
            VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT | VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
            0, 0, nullptr, 0, nullptr, 1, &barrier);

        VulkanContext::Get()->EndSingleTimeCommands(cmd);

        // Make per-layer views with the SAME format as the image (UINT!)
        m_propsLayerPool.Init(dev, m_propsArrayImage, VK_FORMAT_R8G8B8A8_UINT, m_arrayLayerCount);
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



    // ----- Descriptor writes
    void VulkanBindlessDescriptorSetRenderer::WriteCombinedImageSampler( VkDevice device, VkDescriptorSet set, uint32_t binding,
        uint32_t arrayIndex, VkSampler sampler, VkImageView view, VkImageLayout layout)
    {
        EE_PROFILE_FUNCTION();

        
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
        w.dstSet = set; w.dstBinding = 2;
        w.dstArrayElement = 0;
        w.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        w.descriptorCount = 1;
        w.pBufferInfo = &info;
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


    void VulkanBindlessDescriptorSetRenderer::EvictAllTiles()
    {
        EE_PROFILE_FUNCTION();

        if (m_tileToSlot.empty())
            return;

      
        for (const auto& [uid, slot] : m_tileToSlot)
        {
            (void)uid;

       

            m_colorLayerPool.Release(slot);
            m_propsLayerPool.Release(slot);
        }

        m_tileToSlot.clear();
    }



    uint32_t VulkanBindlessDescriptorSetRenderer::EnsureTileResidentFromRaw(uint64_t uid, const uint8_t* colorData,
        size_t colorSize,  const uint8_t* propsData, size_t propsSize, VkCommandBuffer uploadCB)
    {

        EE_PROFILE_FUNCTION();

        if (auto it = m_tileToSlot.find(uid); it != m_tileToSlot.end())
            return it->second;

        // sanity
        const size_t expect = size_t(m_tileW) * size_t(m_tileH) * 4;
        EE_CORE_ASSERT(colorData && propsData, "null input");
        EE_CORE_ASSERT(colorSize == expect && propsSize == expect, "bad blob sizes");

        const uint32_t slot = m_colorLayerPool.Acquire();
        VkImageView colorView = m_colorLayerPool.View(slot);
        VkImageView propsView = m_propsLayerPool.View(slot);

        // Upload pixels
        UploadToArrayLayerViaStaging_ST(
            colorData, colorSize,
            m_colorArrayImage,
            /*currentLayout*/ VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,  // or VK_IMAGE_LAYOUT_UNDEFINED on very first upload
            /*finalLayout  */ VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
            /*layer*/ slot,
            /*w*/ m_tileW, /*h*/ m_tileH);

        // Props array: STORAGE image for compute, keep layers in GENERAL
        UploadToArrayLayerViaStaging_ST(
            propsData, propsSize,
            m_propsArrayImage,
            /*currentLayout*/ VK_IMAGE_LAYOUT_GENERAL,                   // or VK_IMAGE_LAYOUT_UNDEFINED on very first upload
            /*finalLayout  */ VK_IMAGE_LAYOUT_GENERAL,
            /*layer*/ slot,
            /*w*/ m_tileW, /*h*/ m_tileH);

        // Write descriptors for all frames 
        for (uint32_t f = 0; f < MAX_FRAMES_IN_FLIGHT; ++f)
        {
            WriteCombinedImageSampler(m_device, m_bindlessSet[f], 0, slot, m_tileSampler,
                colorView, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

            ComputeWriteImageSlot(f, slot, colorView, propsView);
        }

        m_tileToSlot.emplace(uid, slot);
        return slot;
    }


    void VulkanBindlessDescriptorSetRenderer::ComputeWriteImageSlot(uint32_t frameIndex,
        uint32_t arrayIndex,      // = slot
        VkImageView colorView,    // rgba8
        VkImageView propsView)    // rgba8ui
    {

        EE_PROFILE_FUNCTION();

        // binding 0: color (STORAGE_IMAGE array)
        VkDescriptorImageInfo color{};
        color.imageView = colorView;
        color.imageLayout = VK_IMAGE_LAYOUT_GENERAL; // compute writes in GENERAL

        VkWriteDescriptorSet w0{ VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET };
        w0.dstSet = m_computeDescriptorSet[frameIndex];
        w0.dstBinding = 0;
        w0.dstArrayElement = arrayIndex;
        w0.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
        w0.descriptorCount = 1;
        w0.pImageInfo = &color;

        // binding 1: props (STORAGE_IMAGE array)
        VkDescriptorImageInfo props{};
        props.imageView = propsView;
        props.imageLayout = VK_IMAGE_LAYOUT_GENERAL;

        VkWriteDescriptorSet w1{ VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET };
        w1.dstSet = m_computeDescriptorSet[frameIndex];
        w1.dstBinding = 1;
        w1.dstArrayElement = arrayIndex;
        w1.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
        w1.descriptorCount = 1;
        w1.pImageInfo = &props;

        VkWriteDescriptorSet writes[2] = { w0, w1 };
        vkUpdateDescriptorSets(m_device, 2, writes, 0, nullptr);

        //***** effects **********
        VkWriteDescriptorSet wEffects0{ VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET };
        wEffects0.dstSet = m_effectsDescriptorSet[frameIndex];
        wEffects0.dstBinding = 0;
        wEffects0.dstArrayElement = arrayIndex;
        wEffects0.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
        wEffects0.descriptorCount = 1;
        wEffects0.pImageInfo = &color;

       
        VkWriteDescriptorSet wEffect1{ VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET };
        wEffect1.dstSet = m_effectsDescriptorSet[frameIndex];
        wEffect1.dstBinding = 1;
        wEffect1.dstArrayElement = arrayIndex;
        wEffect1.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
        wEffect1.descriptorCount = 1;
        wEffect1.pImageInfo = &props;

        VkWriteDescriptorSet writesEffect[2] = { wEffects0, wEffect1 };
        vkUpdateDescriptorSets(m_device, 2, writesEffect, 0, nullptr);

    }

    void VulkanBindlessDescriptorSetRenderer::ComputeBindBuffers(uint32_t frameIndex,
        VkBuffer resultsBuf, VkDeviceSize resultsSize, VkBuffer projectilesBuf, VkDeviceSize projSize,
        VkBuffer blockedMaskBuf, VkDeviceSize maskSize)
    {
        VkDescriptorBufferInfo rbi{ resultsBuf,     0, resultsSize };
        VkDescriptorBufferInfo pbi{ projectilesBuf, 0, projSize };
        VkDescriptorBufferInfo mbi{ blockedMaskBuf, 0, maskSize };

        VkWriteDescriptorSet wr{ VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET };
        wr.dstSet = m_computeDescriptorSet[frameIndex];
        wr.dstBinding = 2;
        wr.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        wr.descriptorCount = 1;
        wr.pBufferInfo = &rbi;

        VkWriteDescriptorSet wp{ VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET };
        wp.dstSet = m_computeDescriptorSet[frameIndex];
        wp.dstBinding = 3;
        wp.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        wp.descriptorCount = 1;
        wp.pBufferInfo = &pbi;

        VkWriteDescriptorSet wm{ VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET };
        wm.dstSet = m_computeDescriptorSet[frameIndex];
        wm.dstBinding = 4;
        wm.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        wm.descriptorCount = 1;
        wm.pBufferInfo = &mbi;

        VkWriteDescriptorSet writes[3] = { wr, wp, wm };
        vkUpdateDescriptorSets(m_device, 3, writes, 0, nullptr);
    }


    void VulkanBindlessDescriptorSetRenderer::EffectsBindBuffers(uint32_t frameIndex,
        VkBuffer resultsBuf, VkDeviceSize resultsSize, VkBuffer projectilesBuf, VkDeviceSize projSize,
        VkBuffer blockedMaskBuf, VkDeviceSize maskSize)
    {
        VkDescriptorBufferInfo rbi{ resultsBuf,     0, resultsSize };
        VkDescriptorBufferInfo pbi{ projectilesBuf, 0, projSize };
        VkDescriptorBufferInfo mbi{ blockedMaskBuf, 0, maskSize };

        VkWriteDescriptorSet wr{ VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET };
        wr.dstSet = m_effectsDescriptorSet[frameIndex];
        wr.dstBinding = 2;
        wr.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        wr.descriptorCount = 1;
        wr.pBufferInfo = &rbi;

        VkWriteDescriptorSet wp{ VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET };
        wp.dstSet = m_effectsDescriptorSet[frameIndex];
        wp.dstBinding = 3;
        wp.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        wp.descriptorCount = 1;
        wp.pBufferInfo = &pbi;

        VkWriteDescriptorSet wm{ VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET };
        wm.dstSet = m_effectsDescriptorSet[frameIndex];
        wm.dstBinding = 4;
        wm.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        wm.descriptorCount = 1;
        wm.pBufferInfo = &mbi;

        VkWriteDescriptorSet writes[3] = { wr, wp, wm };
        vkUpdateDescriptorSets(m_device, 3, writes, 0, nullptr);
    }

    void VulkanBindlessDescriptorSetRenderer::UpdateCollisionResultDescriptor(uint32_t frameIndex,
        VkDescriptorSet dstSet, VkBuffer resultsBuf)
    {
       

        VkDescriptorBufferInfo dbi{};
        dbi.buffer = resultsBuf;
        dbi.offset = 0;
        dbi.range = sizeof(CollisionResultBuffer);

        VkWriteDescriptorSet write{};
        write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        write.dstSet = dstSet;
        write.dstBinding = 2;
        write.dstArrayElement = 0;
        write.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        write.descriptorCount = 1;
        write.pBufferInfo = &dbi;

        vkUpdateDescriptorSets(m_device, 1, &write, 0, nullptr);
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

    // Upload a tightly-packed RGBA8 (numBytes = width*height*4) into dstImage array[layer].
    // - cmd must be a begun command buffer OUTSIDE any render pass.
    // - finalLayout is usually VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL for color,
    void VulkanBindlessDescriptorSetRenderer::UploadToArrayLayerViaStaging_ST(
        const void* srcData, size_t numBytes,
        VkImage dstImage, VkImageLayout currentLayout, VkImageLayout finalLayout,
        uint32_t layer, uint32_t width, uint32_t height)
    {
        EE_PROFILE_FUNCTION();

        VkDevice device = m_device;
        const size_t expected = size_t(width) * size_t(height) * 4;
        if (!dstImage)
        {
            EE_CORE_ERROR("UploadToArrayLayerViaStaging_ST: dstImage is null");
            return;
        }

        if (width == 0 || height == 0)
        {
            EE_CORE_ERROR("UploadToArrayLayerViaStaging_ST: width/height are zero (w={}, h={})",
                width, height);
            return;
        }

        if (numBytes < expected)
        {
            EE_CORE_ERROR("UploadToArrayLayerViaStaging_ST: upload size too small. numBytes={}, expected={}",
                numBytes, expected);
            return;
        }

        // 1) staging buffer
        VkBuffer staging = VK_NULL_HANDLE;
        VkDeviceMemory stagingMem = VK_NULL_HANDLE;

        VkBufferCreateInfo bc = { VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
        bc.size = expected;
        bc.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
        bc.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        VkResult res = vkCreateBuffer(device, &bc, nullptr, &staging);
        if (res != VK_SUCCESS) 
        {
            EE_CORE_ERROR("UploadToArrayLayerViaStaging_ST: vkCreateBuffer failed (res={}) for size={}", (int)res, (unsigned long long)expected);
            return;
        }

       

        VkMemoryRequirements req{};
        vkGetBufferMemoryRequirements(device, staging, &req);

        

        uint32_t typeIndex = VulkanContext::Get()->FindMemoryType(
            req.memoryTypeBits,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

        if (typeIndex == UINT32_MAX)
        {
            EE_CORE_ERROR("UploadToArrayLayerViaStaging_ST: no suitable memory type");
            vkDestroyBuffer(device, staging, nullptr);
            return;
        }

        VkMemoryAllocateInfo allocInfo{ VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO };
        allocInfo.allocationSize = req.size;
        allocInfo.memoryTypeIndex = typeIndex;

        res = vkAllocateMemory(device, &allocInfo, nullptr, &stagingMem);
        if (res != VK_SUCCESS)
        {
            EE_CORE_ERROR("UploadToArrayLayerViaStaging_ST: vkAllocateMemory failed (res={}), size={}",
                (int)res, (unsigned long long)req.size);
            vkDestroyBuffer(device, staging, nullptr);
            return;
        }

        res = vkBindBufferMemory(device, staging, stagingMem, 0);
        if (res != VK_SUCCESS)
        {
            EE_CORE_ERROR("UploadToArrayLayerViaStaging_ST: vkBindBufferMemory failed (res={})", (int)res);
            vkFreeMemory(device, stagingMem, nullptr);
            vkDestroyBuffer(device, staging, nullptr);
            return;
        }

        // 2) upload data
        void* dstPtr = nullptr;
        res = vkMapMemory(device, stagingMem, 0, expected, 0, &dstPtr);
        if (res != VK_SUCCESS)
        {
            EE_CORE_ERROR("UploadToArrayLayerViaStaging_ST: vkMapMemory failed (res={})", (int)res);
            vkFreeMemory(device, stagingMem, nullptr);
            vkDestroyBuffer(device, staging, nullptr);
            return;
        }

        std::memcpy(dstPtr, srcData, expected);
        vkUnmapMemory(device, stagingMem);
        // 2) begin one-off CB
        VkCommandBuffer cmd = VulkanContext::Get()->BeginSingleTimeCommands();

        // 3) layer -> TRANSFER_DST_OPTIMAL (from currentLayout you pass in)
        {
            VkImageMemoryBarrier b = { VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER };
            b.srcAccessMask = 0;
            b.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
            b.oldLayout = currentLayout;                   // pass UNDEFINED for first upload, GENERAL/READ_ONLY on reupload
            b.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
            b.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            b.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            b.image = dstImage;
            b.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            b.subresourceRange.baseMipLevel = 0;
            b.subresourceRange.levelCount = 1;
            b.subresourceRange.baseArrayLayer = layer;
            b.subresourceRange.layerCount = 1;

            vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
                0, 0, nullptr, 0, nullptr, 1, &b);
        }

        // 4) copy buffer -> image (that layer)
        VkBufferImageCopy region = {};
        region.bufferOffset = 0;
        region.bufferRowLength = 0;     // tightly packed
        region.bufferImageHeight = 0;   // tightly packed
        region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        region.imageSubresource.mipLevel = 0;
        region.imageSubresource.baseArrayLayer = layer;
        region.imageSubresource.layerCount = 1;
        region.imageOffset = { 0, 0, 0 };
        region.imageExtent = { width, height, 1 };

        vkCmdCopyBufferToImage(cmd, staging, dstImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

        // 5) layer -> finalLayout (READ_ONLY for color array; GENERAL for props array)
        {
            VkImageMemoryBarrier b = { VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER };
            b.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
            b.dstAccessMask = (finalLayout == VK_IMAGE_LAYOUT_GENERAL)
                ? (VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT)
                : VK_ACCESS_SHADER_READ_BIT;
            b.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
            b.newLayout = finalLayout;
            b.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            b.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            b.image = dstImage;
            b.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            b.subresourceRange.baseMipLevel = 0;
            b.subresourceRange.levelCount = 1;
            b.subresourceRange.baseArrayLayer = layer;
            b.subresourceRange.layerCount = 1;

            VkPipelineStageFlags dstStage = (finalLayout == VK_IMAGE_LAYOUT_GENERAL)
                ? (VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT | VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT)
                : VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;

            vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TRANSFER_BIT, dstStage, 0, 0, nullptr, 0, nullptr, 1, &b);
        }

        // 6) submit+wait (this ensures GPU is done with 'staging'), then free staging.
        VulkanContext::Get()->EndSingleTimeCommands(cmd);
        vkDestroyBuffer(device, staging, nullptr);
        vkFreeMemory(device, stagingMem, nullptr);
    }

    void VulkanBindlessDescriptorSetRenderer::UpdateEffectImageDescriptorSets(size_t frameIndex, const std::array<Ref<VulkanTexture>, CHUNK_GRID_WIDTH* CHUNK_GRID_WIDTH>& textures)
    {
        std::array<VkDescriptorImageInfo, CHUNK_GRID_WIDTH* CHUNK_GRID_WIDTH> imageInfos{};
        for (uint32_t i = 0; i < CHUNK_GRID_WIDTH * CHUNK_GRID_WIDTH; ++i)
        {
           // uint32_t usedTextureSlots = 1; // player only. This is crap. change it.
            //uint32_t index = i + CHUNK_GRID_SIZE + usedTextureSlots;
            imageInfos[i].imageLayout = VK_IMAGE_LAYOUT_GENERAL;
            imageInfos[i].imageView = textures[i]->GetImageView();
            imageInfos[i].sampler = VK_NULL_HANDLE;

        }


        VkWriteDescriptorSet write{};
        write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        write.dstSet = m_effectsDescriptorSet[frameIndex];
        write.dstBinding = 5;
        write.dstArrayElement = 0;
        write.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
        write.descriptorCount = static_cast<uint32_t>(imageInfos.size());
        write.pImageInfo = imageInfos.data();

        vkUpdateDescriptorSets(m_device, 1, &write, 0, nullptr);
    }

    void VulkanBindlessDescriptorSetRenderer::UpdateLightBufferDescriptor(uint32_t frameIndex)
    {
        const uint32_t fi = 0;
        Ref<VulkanBuffer> lightBuffer = VulkanLighting::GetLightBuffer();

        if (!lightBuffer)
        {
            EE_CORE_WARN("Light buffer not initialized for frame {}");
            return;
        }

        VkDescriptorBufferInfo bufferInfo{};
        bufferInfo.buffer = lightBuffer->GetBuffer();
        bufferInfo.offset = 0;
        bufferInfo.range = sizeof(GPULightBuffer);



        VkWriteDescriptorSet descriptorWrite{};
        descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrite.dstSet = m_bindlessSet[frameIndex];
        descriptorWrite.dstBinding = 4;
        descriptorWrite.dstArrayElement = 0;
        descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        descriptorWrite.descriptorCount = 1;
        descriptorWrite.pBufferInfo = &bufferInfo;


        vkUpdateDescriptorSets(m_device, 1, &descriptorWrite, 0, nullptr);
    }


    bool VulkanBindlessDescriptorSetRenderer::IsInsideView(const Camera&, glm::vec2) const
    {
        // TODO: plug real culling
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

    // In your bindless tiles renderer
    void VulkanBindlessDescriptorSetRenderer::DrawTilesShadowPass(VkCommandBuffer cmd, uint32_t frameIndex,
        VkPipeline shadowPipeline, VkPipelineLayout shadowPipelineLayout,
        const glm::mat4& lightSpaceMatrix)
    {
        if (m_drawCount == 0) return;

        // Bind shadow pipeline
        vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, shadowPipeline);

        // Bind instance buffer descriptor set
        vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS,
            shadowPipelineLayout, 0, 1, &m_bindlessSet[frameIndex], 0, nullptr);

        // Push light space matrix
        vkCmdPushConstants(cmd, shadowPipelineLayout,
            VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(glm::mat4), &lightSpaceMatrix);

        // Draw instanced (no vertex/index buffers - uses gl_VertexIndex/gl_InstanceIndex)
        vkCmdDraw(cmd, 4, m_drawCount, 0, 0);  // 4 vertices per quad, N instances
    }

   
    uint32_t VulkanBindlessDescriptorSetRenderer::EnsureTileResident(uint64_t uid,  const glm::vec4& atlasUV,
        VkCommandBuffer uploadCmd)
    {
        if (auto it = m_tileToSlot.find(uid); it != m_tileToSlot.end())
            return it->second;

        const uint32_t layer = m_colorLayerPool.Acquire();
        VkImageView colorView = m_colorLayerPool.View(layer);

      
        VkImage atlasImg = AssetManager::GetTileTextureIconAtlas()->GetImage();
        CopyFromAtlasUVToLayer(uploadCmd,
            atlasImg,
            atlasUV,              // expects uv.x/uv.y to be pixel-aligned top-left
            m_colorArrayImage,
            layer,
            m_tileW, m_tileH);

      
        for (uint32_t f = 0; f < MAX_FRAMES_IN_FLIGHT; ++f)
        {
            
            WriteCombinedImageSampler(m_device, m_bindlessSet[f],
                /*binding*/0, /*arrayIndex*/layer,
                m_tileSampler, colorView,
                VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

           
        }

        m_tileToSlot.emplace(uid, layer);
        return layer;
    }

    void VulkanBindlessDescriptorSetRenderer::CreateComputeDescriptorSet(VkDescriptorPool computeDescriptorPool)
    {


        std::vector<VkDescriptorSetLayout> layouts(MAX_FRAMES_IN_FLIGHT, m_computeDescriptorSetLayout);

        VkDescriptorSetAllocateInfo ai{ VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO };
        ai.descriptorPool = computeDescriptorPool;
        ai.descriptorSetCount = MAX_FRAMES_IN_FLIGHT;
        ai.pSetLayouts = layouts.data();

        VkResult r = vkAllocateDescriptorSets(m_device, &ai, m_computeDescriptorSet.data());
        EE_CORE_ASSERT(r == VK_SUCCESS, "Failed to allocate compute descriptor sets");
    }

    void VulkanBindlessDescriptorSetRenderer::CreateComputeArrayDescriptorSetLayout(
        uint32_t maxResidentLayers, bool updateAfterBindSupported)
    {
        // 0: COLOR  (rgba8)   STORAGE image, bindless array
        // 1: PROPS  (rgba8ui) STORAGE image, bindless array
        // 2: ResultBuffer      SSBO (1)
        // 3: Projectiles       SSBO (1)
        // 4: BlockedTileMask   SSBO (1)
        // 5: effects  (rgba8ui) STORAGE image, array

        std::array<VkDescriptorSetLayoutBinding, 6> bindings{};

        // binding 0: color image array (compute writes)
        bindings[0].binding = 0;
        bindings[0].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
        bindings[0].descriptorCount = maxResidentLayers;     // bindless array size
        bindings[0].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
        bindings[0].pImmutableSamplers = nullptr;

        // binding 1: properties/health image array (compute reads/writes)
        bindings[1].binding = 1;
        bindings[1].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
        bindings[1].descriptorCount = maxResidentLayers;     // bindless array size
        bindings[1].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
        bindings[1].pImmutableSamplers = nullptr;

        // binding 2: results SSBO
        bindings[2].binding = 2;
        bindings[2].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        bindings[2].descriptorCount = 1;
        bindings[2].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
        bindings[2].pImmutableSamplers = nullptr;

        // binding 3: projectiles SSBO
        bindings[3].binding = 3;
        bindings[3].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        bindings[3].descriptorCount = 1;
        bindings[3].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
        bindings[3].pImmutableSamplers = nullptr;

        // binding 4: blocked mask SSBO
        bindings[4].binding = 4;
        bindings[4].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        bindings[4].descriptorCount = 1;
        bindings[4].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
        bindings[4].pImmutableSamplers = nullptr;

        // effects
        bindings[5].binding = 5;
        bindings[5].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
        bindings[5].descriptorCount = CHUNK_GRID_WIDTH * CHUNK_GRID_WIDTH; // 3x3 grid 
        bindings[5].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
        bindings[5].pImmutableSamplers = nullptr;

        // Descriptor indexing binding flags:
        // - PARTIALLY_BOUND lets you leave unused array elements unwritten.
        // - UPDATE_AFTER_BIND is optional; only use if you actually enabled it at device creation.
        std::array<VkDescriptorBindingFlags, 6> bflags{};
        bflags[0] = VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT |
            (updateAfterBindSupported ? VK_DESCRIPTOR_BINDING_UPDATE_AFTER_BIND_BIT : 0);
        bflags[1] = VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT |
            (updateAfterBindSupported ? VK_DESCRIPTOR_BINDING_UPDATE_AFTER_BIND_BIT : 0);
        // buffers don’t need special flags
        bflags[2] = 0;
        bflags[3] = 0;
        bflags[4] = 0;
        bflags[5] = 0;

        VkDescriptorSetLayoutBindingFlagsCreateInfo flagsCI{
            VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO
        };
        flagsCI.bindingCount = static_cast<uint32_t>(bflags.size());
        flagsCI.pBindingFlags = bflags.data();

        VkDescriptorSetLayoutCreateInfo layoutInfo{
            VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO
        };

        layoutInfo.pNext = &flagsCI;                             // descriptor indexing flags
        layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
        layoutInfo.pBindings = bindings.data();
        layoutInfo.flags = updateAfterBindSupported
            ? VK_DESCRIPTOR_SET_LAYOUT_CREATE_UPDATE_AFTER_BIND_POOL_BIT
            : 0;

        VkResult res = vkCreateDescriptorSetLayout(m_device, &layoutInfo, nullptr, &m_computeDescriptorSetLayout);
        EE_CORE_ASSERT(res == VK_SUCCESS, "Failed to create compute descriptor set layout");

      
    }



    void VulkanBindlessDescriptorSetRenderer::CreateComputeGraphicsPipeline()
    {
        VkPipelineShaderStageCreateInfo computeShaderStageInfo{};
        computeShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        computeShaderStageInfo.stage = VK_SHADER_STAGE_COMPUTE_BIT;
        computeShaderStageInfo.module = m_computeShader->GetComputeshaderModule();
        computeShaderStageInfo.pName = "main";

        VkPushConstantRange pushConstantRange{};
        pushConstantRange.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
        pushConstantRange.offset = 0;
        pushConstantRange.size = sizeof(ComputePC);

        VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
        pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        pipelineLayoutInfo.setLayoutCount = 1;
        pipelineLayoutInfo.pSetLayouts = &m_computeDescriptorSetLayout;
        pipelineLayoutInfo.pushConstantRangeCount = 1;
        pipelineLayoutInfo.pPushConstantRanges = &pushConstantRange;

        vkCreatePipelineLayout(m_device, &pipelineLayoutInfo, nullptr, &m_computePipelineLayout);

        VkComputePipelineCreateInfo computePipelineInfo{};
        computePipelineInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
        computePipelineInfo.stage = computeShaderStageInfo;
        computePipelineInfo.layout = m_computePipelineLayout;

        vkCreateComputePipelines(m_device, VK_NULL_HANDLE, 1, &computePipelineInfo, nullptr, &m_computePipeline);
    }




    void VulkanBindlessDescriptorSetRenderer::CreateEffectsPipeline()
    {

        // Describe the shader stage
        VkPipelineShaderStageCreateInfo shaderStageInfo{};
        shaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        shaderStageInfo.stage = VK_SHADER_STAGE_COMPUTE_BIT;
        shaderStageInfo.module = m_effectShader->GetComputeshaderModule();
        shaderStageInfo.pName = "main";

        // Set up the compute pipeline creation info
        VkComputePipelineCreateInfo pipelineInfo{};
        pipelineInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
        pipelineInfo.stage = shaderStageInfo;
        pipelineInfo.layout = m_effectsPipelineLayout;
        pipelineInfo.flags = 0;

        if (vkCreateComputePipelines(m_device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &m_effectsPipeline) != VK_SUCCESS)
        {
            EE_CORE_ASSERT(false, "failed to create effects graphics pipeline!");
        }

        // Clean up shader module after pipeline creation
        vkDestroyShaderModule(m_device, m_effectShader->GetComputeshaderModule(), nullptr);
    }

    void VulkanBindlessDescriptorSetRenderer::CreateEffectsPipelineLayout()
    {
        // Define the push constant range used by the compute shader
        VkPushConstantRange pushRange{};
        pushRange.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
        pushRange.offset = 0;
        pushRange.size = sizeof(EffectPushConstants);

        VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
        pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        pipelineLayoutInfo.setLayoutCount = 1;
        pipelineLayoutInfo.pSetLayouts = &m_computeDescriptorSetLayout;
        pipelineLayoutInfo.pushConstantRangeCount = 1;
        pipelineLayoutInfo.pPushConstantRanges = &pushRange;

        if (vkCreatePipelineLayout(m_device, &pipelineLayoutInfo, nullptr, &m_effectsPipelineLayout) != VK_SUCCESS)
        {
            EE_CORE_ASSERT(false, "failed to create effects pipeline layout!");
        }
    }


    void VulkanBindlessDescriptorSetRenderer::CreateEffectsDescriptorSetLayout()
    {
        // Reuse the SAME set as your compute pass, just append binding 5 for FX grid.
        // (If this is the compute set layout itself, include all bindings 0..5 here.)

        VkDescriptorSetLayoutBinding colorBinding{};
        colorBinding.binding = 0;
        colorBinding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
        colorBinding.descriptorCount = MAX_RESIDENT_LAYERS;
        colorBinding.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

        VkDescriptorSetLayoutBinding propsBinding{};
        propsBinding.binding = 1;
        propsBinding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
        propsBinding.descriptorCount = MAX_RESIDENT_LAYERS;
        propsBinding.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

        VkDescriptorSetLayoutBinding resultsBinding{};
        resultsBinding.binding = 2;                   // SSBO
        resultsBinding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        resultsBinding.descriptorCount = 1;
        resultsBinding.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

        VkDescriptorSetLayoutBinding projectilesBinding{};
        projectilesBinding.binding = 3;               // SSBO
        projectilesBinding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        projectilesBinding.descriptorCount = 1;
        projectilesBinding.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

        VkDescriptorSetLayoutBinding blockedMaskBinding{};
        blockedMaskBinding.binding = 4;               // SSBO
        blockedMaskBinding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        blockedMaskBinding.descriptorCount = 1;
        blockedMaskBinding.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

        VkDescriptorSetLayoutBinding fxGridBinding{};
        fxGridBinding.binding = 5;
        fxGridBinding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
        fxGridBinding.descriptorCount = 9;            // exactly 9
        fxGridBinding.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

        std::array<VkDescriptorSetLayoutBinding, 6> bindings = {
            colorBinding, propsBinding, resultsBinding,
            projectilesBinding, blockedMaskBinding, fxGridBinding
        };

        VkDescriptorSetLayoutCreateInfo info{ VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO };
        info.bindingCount = (uint32_t)bindings.size();
        info.pBindings = bindings.data();

        if (vkCreateDescriptorSetLayout(m_device, &info, nullptr, &m_effectsDescriptorSetLayout) != VK_SUCCESS)
        {
            EE_CORE_ASSERT(false, "Failed to create effects descriptor set layout!");
        }

    }

    void VulkanBindlessDescriptorSetRenderer::CreateEffectsDescriptorSets(VkDescriptorPool computeDescriptorPool)
    {

        std::vector<VkDescriptorSetLayout> layouts(MAX_FRAMES_IN_FLIGHT, m_computeDescriptorSetLayout);

        VkDescriptorSetAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        allocInfo.descriptorPool = computeDescriptorPool;
        allocInfo.descriptorSetCount = MAX_FRAMES_IN_FLIGHT;
        allocInfo.pSetLayouts = layouts.data();

        if (vkAllocateDescriptorSets(m_device, &allocInfo, m_effectsDescriptorSet.data()) != VK_SUCCESS)
        {
            throw std::runtime_error("failed to allocate effects descriptor sets");
        }

    }


    uint32_t VulkanBindlessDescriptorSetRenderer::AllocSpriteSlot_() {
        if (!m_freeSpriteSlots.empty()) {
            uint32_t s = m_freeSpriteSlots.back();
            m_freeSpriteSlots.pop_back();
            return s;
        }
        if (m_nextSpriteSlot >= MAX_SPRITESHEETS) return 0xFFFFFFFFu;
        return m_nextSpriteSlot++;
    }

    void VulkanBindlessDescriptorSetRenderer::WriteSpriteDescriptorAllFrames_(uint32_t slot, VkImageView view)
    {
        for (uint32_t fi = 0; fi < FRAMES_IN_FLIGHT; ++fi) {
            WriteCombinedImageSampler(
                m_device,
                m_bindlessSet[fi],
                /*binding*/ 3,              // <- spritesheets binding
                /*arrayIndex*/ slot,
                /*sampler*/   m_tileSampler, // or a dedicated sprite sampler if you add one
                /*view*/      view,
                VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
            );
        }
    }

    bool VulkanBindlessDescriptorSetRenderer::AcquireSpritesheet(
        const std::string& textureName, uint32_t& outSlot, uint32_t& outW, uint32_t& outH)
    {
        // Normalize to whatever key AssetManager uses
        const std::string key = textureName;

        // Reuse if present
        if (auto it = m_spriteByPath.find(key); it != m_spriteByPath.end()) {
            SpriteRec& rec = it->second;
            rec.refcount++;
            outSlot = rec.slot; outW = rec.w; outH = rec.h;
            return true;
        }

        // Load GPU texture via AssetManager (no renderer coupling in AssetManager)
        Ref<VulkanTexture> tex = AssetManager::GetTexture(key);
        if (!tex) tex = AssetManager::AddTexture(key, key, /*imGui*/ false);
        if (!tex) {
            EE_CORE_ERROR("[BindlessRenderer] AcquireSpritesheet: failed to load '{}'", key);
            return false;
        }

        // Allocate a binding=3 slot
        const uint32_t slot = AllocSpriteSlot_();
        if (slot == 0xFFFFFFFFu) {
            EE_CORE_ERROR("[BindlessRenderer] Spritesheet capacity exceeded (max {})", MAX_SPRITESHEETS);
            return false;
        }

        // Write descriptors for all frames at binding=3
        WriteSpriteDescriptorAllFrames_(slot, tex->GetImageView());

        // Record
        SpriteRec rec{};
        rec.slot = slot; rec.w = tex->GetWidth(); rec.h = tex->GetHeight();
        rec.refcount = 1; rec.tex = tex;

        m_spritePathBySlot[slot] = key;
        m_spriteByPath.emplace(key, std::move(rec));

        outSlot = slot; outW = tex->GetWidth(); outH = tex->GetHeight();
        return true;
    }

    void VulkanBindlessDescriptorSetRenderer::ReleaseSpritesheet(uint32_t slot)
    {
        auto itPath = m_spritePathBySlot.find(slot);
        if (itPath == m_spritePathBySlot.end()) return;

        auto it = m_spriteByPath.find(itPath->second);
        if (it == m_spriteByPath.end()) return;

        SpriteRec& rec = it->second;
        if (rec.refcount > 0) rec.refcount--;
        if (rec.refcount) return;

        // Optional: clear descriptor to a dummy 1x1 view (not strictly required)
        // For now, just recycle the slot
        m_freeSpriteSlots.push_back(rec.slot);
        m_spritePathBySlot.erase(itPath);
        m_spriteByPath.erase(it);
    }

} 