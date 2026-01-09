#include "pch.h"
#include "VulkanUIRenderer.h"
#include "VulkanUIGraphicsPipeline.h"

#include <glm/gtc/matrix_transform.hpp>
#include <cstring>
#include <Engine/Platform/Vulkan/VulkanContext.h>
#include <Engine/Platform/Vulkan/VulkanBuffer.h>
#include "Engine/UI/UIUtils/UIUtils.h"

namespace Engine
{
    VulkanUIRenderer* VulkanUIRenderer::s_active;


    static uint64_t TexKey(const Ref<VulkanTexture>& t)
    {
        return (uint64_t)(uintptr_t)t.get();
    }

   

    VulkanUIRenderer::VulkanUIRenderer(VulkanContext* vulkanContext)
        : m_vulkanContext(vulkanContext)
   
    {
       
    }

    VulkanUIRenderer::~VulkanUIRenderer()
    {
        Shutdown();
    }

    void VulkanUIRenderer::Init(const UIRendererInitConfig& cfg, VulkanContext* vulkanContext)
    {
        m_vulkanContext = vulkanContext;
        m_device = m_vulkanContext->GetDeviceManager().GetDevice();
        m_fallbackWhite = std::make_shared<VulkanTexture>(1, 1, VK_FORMAT_R8G8B8A8_UINT);
        m_uIGraphicsPipeline = std::make_shared<VulkanUIGraphicsPipeline>(m_device, m_vulkanContext->GetDeviceManager().GetPhysicalDevice());

        VkExtent2D swapchainExtent = m_vulkanContext->GetVulkanSwapchain().GetSwapchainExtent();

        Engine::VulkanUIGraphicsPipeline::UIInitConfig uiInitConfig;
        uiInitConfig.viewportWidth = swapchainExtent.width;
        uiInitConfig.viewportHeight = swapchainExtent.height;
        uiInitConfig.renderPass = m_vulkanContext->GetGameRenderPass();

        m_uIGraphicsPipeline->Init(uiInitConfig, MAX_FRAMES_IN_FLIGHT);


        m_cfg = cfg;

        const uint32_t maxVerts = m_cfg.maxQuads * 4;
        const uint32_t maxIndices = m_cfg.maxQuads * 6;

        m_cpuVertices.resize(maxVerts);

        // Texture slots must match pipeline maxTextures
        m_textureSlots.resize(m_cfg.maxTextures);
        for (uint32_t i = 0; i < m_cfg.maxTextures; ++i)
            m_textureSlots[i] = m_fallbackWhite;

        m_vertexBuffer = std::make_shared<VulkanBuffer>(
            m_device,
            m_vulkanContext->GetDeviceManager().GetPhysicalDevice(),
            sizeof(Engine::VulkanUIGraphicsPipeline::VulkanUIQuadVertex) * maxVerts,
            VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);


        std::vector<uint32_t> indices(maxIndices);

        uint32_t offset = 0;
        for (uint32_t i = 0; i < maxIndices; i += 6)
        {
            indices[i + 0] = offset + 0;
            indices[i + 1] = offset + 1;
            indices[i + 2] = offset + 2;

            indices[i + 3] = offset + 2;
            indices[i + 4] = offset + 3;
            indices[i + 5] = offset + 0;

            offset += 4;
        }



        m_indexBuffer = std::make_shared<VulkanIndexBuffer>(indices.data(), indices.size());
        m_initialized = true;
    }

    void VulkanUIRenderer::Shutdown()
    {
        if (!m_initialized)
            return;

        m_vertexBuffer.reset();
        m_indexBuffer.reset();

        m_cpuVertices.clear();
        m_textureSlots.clear();
        m_texLUT.clear();

        m_initialized = false;
    }

    void VulkanUIRenderer::Begin(uint32_t frameIndex, glm::vec2 viewportPx)
    {

        m_frameIndex = frameIndex;
        m_viewportPx = viewportPx;
        ResetBatch();
    }

    void VulkanUIRenderer::EndAndRecord(VkCommandBuffer cmd)
    {
        if (m_indexCount == 0)
            return;

        UploadVB();
        UpdateDescriptors();
        BindAndDraw(cmd);
    }

    void VulkanUIRenderer::ResetBatch()
    {
        m_indexCount = 0;
        m_vtxPtr = m_cpuVertices.data();

        m_textureSlotCount = 0;
        m_texLUT.clear();

     
        for (uint32_t i = 0; i < m_cfg.maxTextures; ++i)
            m_textureSlots[i] = m_fallbackWhite;
    }

    glm::mat4 VulkanUIRenderer::MakeUIVP(glm::vec2 viewportPx) const
    {
        // Top-left origin (0,0), y down
        return glm::ortho(0.0f, viewportPx.x, viewportPx.y, 0.0f, -1.0f, 1.0f);
    }

    uint32_t VulkanUIRenderer::AcquireTextureSlot(const Ref<VulkanTexture>& texture)
    {
        Ref<VulkanTexture> t = texture ? texture : m_fallbackWhite;
        const uint64_t key = TexKey(t);

        auto it = m_texLUT.find(key);
        if (it != m_texLUT.end())
            return it->second;

        if (m_textureSlotCount >= m_cfg.maxTextures)
        {
            // should  Flush -> Reset -> continue.
            return 0;
        }

        const uint32_t slot = m_textureSlotCount++;
        m_textureSlots[slot] = t;
        m_texLUT[key] = slot;
        return slot;
    }



    void VulkanUIRenderer::DrawUIIcon_Impl(const Ref<VulkanTexture>& icon, UITransform2D tr , glm::vec4 tint)
    {

        glm::vec2 topLeft = UIUtils::ComputeTopLeftPx(tr, m_viewportPx);


        const float x0 = topLeft.x;
        const float y0 = topLeft.y;
        const float x1 = x0 + tr.sizePx.x;
        const float y1 = y0 + tr.sizePx.y;

        glm::vec3 p0(x0, y0, 0.0f); // TL
        glm::vec3 p1(x1, y0, 0.0f); // TR
        glm::vec3 p2(x1, y1, 0.0f); // BR
        glm::vec3 p3(x0, y1, 0.0f); // BL

        glm::vec2 uv0(0.0f, 0.0f);
        glm::vec2 uv1(1.0f, 0.0f);
        glm::vec2 uv2(1.0f, 1.0f);
        glm::vec2 uv3(0.0f, 1.0f);

        DrawQuadRaw(p0, p1, p2, p3, uv0, uv1, uv2, uv3, icon, tint);
    }



    void VulkanUIRenderer::DrawQuadRaw(const glm::vec3& p0, const glm::vec3& p1, const glm::vec3& p2, const glm::vec3& p3,
        const glm::vec2& uv0, const glm::vec2& uv1, const glm::vec2& uv2, const glm::vec2& uv3,
        const Ref<VulkanTexture>& texture, const glm::vec4& tintColor)
    {
        const uint32_t maxIndices = m_cfg.maxQuads * 6;
        if (m_indexCount + 6 > maxIndices)
        {
            // Flush here and continue.
            return;
        }

        const uint32_t texSlot = AcquireTextureSlot(texture);

        VulkanUIGraphicsPipeline::VulkanUIQuadVertex* v = m_vtxPtr;

        v[0].Position = p0;
        v[0].Color = tintColor;
        v[0].TexCoord = uv0;
        v[0].TexIndex = texSlot;

        v[1].Position = p1;
        v[1].Color = tintColor;
        v[1].TexCoord = uv1;
        v[1].TexIndex = texSlot;

        v[2].Position = p2;
        v[2].Color = tintColor;
        v[2].TexCoord = uv2;
        v[2].TexIndex = texSlot;

        v[3].Position = p3;
        v[3].Color = tintColor;
        v[3].TexCoord = uv3;
        v[3].TexIndex = texSlot;

        m_vtxPtr += 4;
        m_indexCount += 6;
    }

    void VulkanUIRenderer::UploadVB()
    {
        const uint8_t* base = reinterpret_cast<const uint8_t*>(m_cpuVertices.data());
        const uint8_t* ptr = reinterpret_cast<const uint8_t*>(m_vtxPtr);
        const VkDeviceSize dataSize = VkDeviceSize(ptr - base);

        if (dataSize == 0)
            return;

        m_vertexBuffer->SetData(m_cpuVertices.data(), (size_t)dataSize);
    }

    void VulkanUIRenderer::UpdateDescriptors()
    {
        // 1) Camera UBO
        VulkanUIGraphicsPipeline::CameraUBO cam{};
        cam.ViewProjection = MakeUIVP(m_viewportPx);
        m_uIGraphicsPipeline->UpdateCameraUBO(m_frameIndex, cam);

        std::vector<Ref<VulkanTexture>> full = m_textureSlots;
        m_uIGraphicsPipeline->UpdateUITextures(m_frameIndex, full);
    }

    void VulkanUIRenderer::BindAndDraw(VkCommandBuffer cmd)
    {
        VkPipeline pipe = m_uIGraphicsPipeline->GetPipeline();
        VkPipelineLayout layout = m_uIGraphicsPipeline->GetPipelineLayout();

        vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipe);


        Engine::VulkanUIGraphicsPipeline::UIPushConstants pc{};
        pc.u_GlobalAlpha = 1.0f;
        pc.u_RoundRadiusPx = 0.0f;
        pc.u_RoundFeatherPx = 1.0f;
        pc._pad0 = 0.0f;

        pc.u_ClipRectPx = glm::vec4(0.0f, 0.0f, 0.0f, 0.0f);

        vkCmdPushConstants(cmd, layout, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
            0,  sizeof(Engine::VulkanUIGraphicsPipeline::UIPushConstants), &pc);


        VkViewport vp{};
        vp.x = 0;
        vp.y = 0;
        vp.width = m_viewportPx.x;
        vp.height = m_viewportPx.y;
        vp.minDepth = 0.0f;
        vp.maxDepth = 1.0f;
        vkCmdSetViewport(cmd, 0, 1, &vp);

        VkRect2D sc{};
        sc.offset = { (int32_t)0, (int32_t)0 };
        sc.extent = { (uint32_t)m_viewportPx.x, (uint32_t)m_viewportPx.y };
        vkCmdSetScissor(cmd, 0, 1, &sc);

        VkDescriptorSet set0 = m_uIGraphicsPipeline->GetCameraDescriptorSet(m_frameIndex);
        VkDescriptorSet set1 = m_uIGraphicsPipeline->GetUITextureDescriptorSet(m_frameIndex);
        VkDescriptorSet sets[] = { set0, set1 };

        vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, layout,
            0, 2, sets, 0, nullptr);

        VkBuffer vb = m_vertexBuffer->GetBuffer();
        VkDeviceSize offs = 0;
        vkCmdBindVertexBuffers(cmd, 0, 1, &vb, &offs);

        vkCmdBindIndexBuffer(cmd, m_indexBuffer->GetBuffer(), 0, VK_INDEX_TYPE_UINT32);

        vkCmdDrawIndexed(cmd, m_indexCount, 1, 0, 0, 0);

       

    }
}
