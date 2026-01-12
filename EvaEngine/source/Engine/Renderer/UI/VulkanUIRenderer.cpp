#include "pch.h"
#include "VulkanUIRenderer.h"

#include "VulkanUIGraphicsPipeline.h"
#include "VulkanUITextGraphicsPipeline.h"
#include <glm/gtc/matrix_transform.hpp>
#include <Engine/Platform/Vulkan/VulkanContext.h>
#include <Engine/Platform/Vulkan/VulkanBuffer.h>
#include "Engine/UI/UIUtils/UIUtils.h"
#include "Engine/UI/UITransform2D.h"

namespace Engine
{
    VulkanUIRenderer* VulkanUIRenderer::s_active = nullptr;

    static uint64_t TexKey(const Ref<VulkanTexture>& t)
    {
        return (uint64_t)(uintptr_t)t.get();
    }

    VulkanUIRenderer::VulkanUIRenderer()
    {
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
        m_cfg = cfg;

       
        m_fallbackWhiteRGBA = std::make_shared<VulkanTexture>(1, 1, VK_FORMAT_R8G8B8A8_UNORM);
        m_fallbackWhiteR8 = std::make_shared<VulkanTexture>(1, 1, VK_FORMAT_R8_UNORM);

        // Pipelines
        m_iconPipeline = std::make_shared<VulkanUIGraphicsPipeline>(m_device, m_vulkanContext->GetDeviceManager().GetPhysicalDevice());
        m_textPipeline = std::make_shared<VulkanUITextGraphicsPipeline>(m_device, m_vulkanContext->GetDeviceManager().GetPhysicalDevice());

        VkExtent2D swapchainExtent = m_vulkanContext->GetVulkanSwapchain().GetSwapchainExtent();

        VulkanUIGraphicsPipeline::UIInitConfig iconInit{};
        iconInit.viewportWidth = swapchainExtent.width;
        iconInit.viewportHeight = swapchainExtent.height;
        iconInit.renderPass = m_vulkanContext->GetGameRenderPass();
        m_iconPipeline->Init(iconInit, MAX_FRAMES_IN_FLIGHT);

        VulkanUITextGraphicsPipeline::UIInitConfig textInit{};
        textInit.viewportWidth = swapchainExtent.width;
        textInit.viewportHeight = swapchainExtent.height;
        textInit.renderPass = m_vulkanContext->GetGameRenderPass();
        m_textPipeline->Init(textInit, MAX_FRAMES_IN_FLIGHT);

        // Buffers
        const uint32_t maxVerts = m_cfg.maxQuads * 4;
        const uint32_t maxIndices = m_cfg.maxQuads * 6;

        m_iconCPU.resize(maxVerts);
        m_textCPU.resize(maxVerts);

        m_iconTextureSlots.resize(m_cfg.maxTextures, m_fallbackWhiteRGBA);
        m_textTextureSlots.resize(m_cfg.maxTextures, m_fallbackWhiteR8);

        m_iconVB = std::make_shared<VulkanBuffer>(
            m_device,
            m_vulkanContext->GetDeviceManager().GetPhysicalDevice(),
            sizeof(VulkanUIGraphicsPipeline::VulkanUIQuadVertex) * maxVerts,
            VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

        m_textVB = std::make_shared<VulkanBuffer>(m_device,
            m_vulkanContext->GetDeviceManager().GetPhysicalDevice(),
            sizeof(VulkanUIGraphicsPipeline::VulkanUIQuadVertex) * maxVerts,
            VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

        // Shared index buffer
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
        if (!m_initialized) return;

        m_iconVB.reset();
        m_textVB.reset();
        m_indexBuffer.reset();

        m_iconPipeline.reset();
        m_textPipeline.reset();

        m_iconCPU.clear();
        m_textCPU.clear();

        m_iconTextureSlots.clear();
        m_textTextureSlots.clear();

        m_iconTexLUT.clear();
        m_textTexLUT.clear();

        m_initialized = false;
    }

    void VulkanUIRenderer::Begin(uint32_t frameIndex, glm::vec2 viewportPx)
    {
        m_frameIndex = frameIndex;
        m_viewportPx = viewportPx;

        ResetIcons();
        ResetText();
    }

    void VulkanUIRenderer::EndAndRecord(VkCommandBuffer cmd)
    {
        if (m_iconIndexCount > 0)
        {
            UploadIconsVB();
            UpdateIconsDescriptors();
            BindAndDrawIcons(cmd);
        }

        if (m_textIndexCount > 0)
        {
            UploadTextVB();
            UpdateTextDescriptors();
            BindAndDrawText(cmd);
        }
    }

    glm::mat4 VulkanUIRenderer::MakeUIVP(glm::vec2 viewportPx) const
    {
        // Top-left origin (0,0), y down
        return glm::ortho(0.0f, viewportPx.x, viewportPx.y, 0.0f, -1.0f, 1.0f);
    }

    // ICONS
  
    void VulkanUIRenderer::ResetIcons()
    {
        m_iconIndexCount = 0;
        m_iconPtr = m_iconCPU.data();
        m_iconTextureSlotCount = 0;
        m_iconTexLUT.clear();
        for (uint32_t i = 0; i < m_cfg.maxTextures; ++i)
        {
            m_iconTextureSlots[i] = m_fallbackWhiteRGBA;
        }
    }

    uint32_t VulkanUIRenderer::AcquireIconTextureSlot(const Ref<VulkanTexture>& texture)
    {
        Ref<VulkanTexture> t = texture ? texture : m_fallbackWhiteRGBA;
        const uint64_t key = TexKey(t);

        auto it = m_iconTexLUT.find(key);
        if (it != m_iconTexLUT.end())
        {
            return it->second;
        }

        if (m_iconTextureSlotCount >= m_cfg.maxTextures)
            return 0;

        const uint32_t slot = m_iconTextureSlotCount++;
        m_iconTextureSlots[slot] = t;
        m_iconTexLUT[key] = slot;
        return slot;
    }


    void VulkanUIRenderer::DrawUIIcon_Impl(const Ref<VulkanTexture>& icon, const UITransform2D& tr, glm::vec4 tint)
    {
        glm::vec2 topLeft = UIUtils::ComputeTopLeftPx(tr, m_viewportPx);
        
        const float x0 = topLeft.x;
        const float y0 = topLeft.y;
        const float x1 = x0 + tr.sizePx.x;
        const float y1 = y0 + tr.sizePx.y;

        glm::vec3 p0(x0, y0, 0.0f);
        glm::vec3 p1(x1, y0, 0.0f);
        glm::vec3 p2(x1, y1, 0.0f);
        glm::vec3 p3(x0, y1, 0.0f);

        glm::vec2 uv0(0.0f, 0.0f);
        glm::vec2 uv1(1.0f, 0.0f);
        glm::vec2 uv2(1.0f, 1.0f);
        glm::vec2 uv3(0.0f, 1.0f);

        DrawQuadRaw_Icons(p0, p1, p2, p3, uv0, uv1, uv2, uv3, icon, tint);
    }

    void VulkanUIRenderer::DrawQuadRaw_Icons(const glm::vec3& p0, const glm::vec3& p1, const glm::vec3& p2, const glm::vec3& p3,
        const glm::vec2& uv0, const glm::vec2& uv1, const glm::vec2& uv2, const glm::vec2& uv3,
        const Ref<VulkanTexture>& texture, const glm::vec4& tintColor)
    {
        const uint32_t maxIndices = m_cfg.maxQuads * 6;
        if (m_iconIndexCount + 6 > maxIndices)
            return;

        const uint32_t texSlot = AcquireIconTextureSlot(texture);

        VulkanUIGraphicsPipeline::VulkanUIQuadVertex* v = m_iconPtr;

        v[0] = { p0, tintColor, uv0, texSlot };
        v[1] = { p1, tintColor, uv1, texSlot };
        v[2] = { p2, tintColor, uv2, texSlot };
        v[3] = { p3, tintColor, uv3, texSlot };

        m_iconPtr += 4;
        m_iconIndexCount += 6;
    }

    void VulkanUIRenderer::UploadIconsVB()
    {
        const uint8_t* base = reinterpret_cast<const uint8_t*>(m_iconCPU.data());
        const uint8_t* ptr = reinterpret_cast<const uint8_t*>(m_iconPtr);
        const VkDeviceSize dataSize = VkDeviceSize(ptr - base);
        if (dataSize == 0) return;

        m_iconVB->SetData(m_iconCPU.data(), (size_t)dataSize);
    }

    void VulkanUIRenderer::UpdateIconsDescriptors()
    {
        VulkanUIGraphicsPipeline::CameraUBO cam{};
        cam.ViewProjection = MakeUIVP(m_viewportPx);
        m_iconPipeline->UpdateCameraUBO(m_frameIndex, cam);

        std::vector<Ref<VulkanTexture>> full = m_iconTextureSlots;
        m_iconPipeline->UpdateUITextures(m_frameIndex, full);
    }

    void VulkanUIRenderer::BindAndDrawIcons(VkCommandBuffer cmd)
    {
        VkPipeline pipe = m_iconPipeline->GetPipeline();
        VkPipelineLayout layout = m_iconPipeline->GetPipelineLayout();
        vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipe);

        VulkanUIGraphicsPipeline::UIPushConstants pc{};
        pc.u_GlobalAlpha = 1.0f;
        pc.u_RoundRadiusPx = 0.0f;
        pc.u_RoundFeatherPx = 1.0f;
        pc._pad0 = 0.0f;
        pc.u_ClipRectPx = glm::vec4(0, 0, 0, 0);

        vkCmdPushConstants(cmd, layout, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
            0, sizeof(VulkanUIGraphicsPipeline::UIPushConstants), &pc);

        VkViewport vp{};
        vp.x = 0; vp.y = 0;
        vp.width = m_viewportPx.x;
        vp.height = m_viewportPx.y;
        vp.minDepth = 0.0f;
        vp.maxDepth = 1.0f;
        vkCmdSetViewport(cmd, 0, 1, &vp);

        VkRect2D sc{};
        sc.offset = { 0, 0 };
        sc.extent = { (uint32_t)m_viewportPx.x, (uint32_t)m_viewportPx.y };
        vkCmdSetScissor(cmd, 0, 1, &sc);

        VkDescriptorSet set0 = m_iconPipeline->GetCameraDescriptorSet(m_frameIndex);
        VkDescriptorSet set1 = m_iconPipeline->GetUITextureDescriptorSet(m_frameIndex);
        VkDescriptorSet sets[] = { set0, set1 };
        vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, layout, 0, 2, sets, 0, nullptr);

        VkBuffer vb = m_iconVB->GetBuffer();
        VkDeviceSize offs = 0;
        vkCmdBindVertexBuffers(cmd, 0, 1, &vb, &offs);
        vkCmdBindIndexBuffer(cmd, m_indexBuffer->GetBuffer(), 0, VK_INDEX_TYPE_UINT32);

        vkCmdDrawIndexed(cmd, m_iconIndexCount, 1, 0, 0, 0);
    }

    // TEXT

    void VulkanUIRenderer::ResetText()
    {
        m_textIndexCount = 0;
        m_textPtr = m_textCPU.data();
        m_textTextureSlotCount = 0;
        m_textTexLUT.clear();
        for (uint32_t i = 0; i < m_cfg.maxTextures; ++i)
            m_textTextureSlots[i] = m_fallbackWhiteR8;
    }

    uint32_t VulkanUIRenderer::AcquireTextTextureSlot(const Ref<VulkanTexture>& texture)
    {
        Ref<VulkanTexture> t = texture ? texture : m_fallbackWhiteR8;
        const uint64_t key = TexKey(t);

        auto it = m_textTexLUT.find(key);
        if (it != m_textTexLUT.end())
        {
            return it->second;

        }

        if (m_textTextureSlotCount >= m_cfg.maxTextures)
            return 0;

        const uint32_t slot = m_textTextureSlotCount++;
        m_textTextureSlots[slot] = t;
        m_textTexLUT[key] = slot;
        return slot;
    }

    void VulkanUIRenderer::DrawText_Impl(const Ref<Font>& fontRef, std::string_view text,
        const UITransform2D& tr, glm::vec4 color, float scale)
    {
        if (!fontRef || text.empty()) return;


        glm::vec2 topLeftPx = UIUtils::ComputeTopLeftPx(tr, m_viewportPx);


        const Font& font = *fontRef;

        float penX = topLeftPx.x;
        float penY = topLeftPx.y + font.ascentPx * scale; // baseline

        for (char c : text)
        {
            if (c == '\n')
            {
                penX = topLeftPx.x;
                penY += (font.ascentPx - font.descentPx + font.lineGapPx) * scale;
                continue;
            }

            unsigned char uc = (unsigned char)c;
            if (uc < 32 || uc >= 127)
                continue;

            const Glyph& g = font.glyphs[uc];

            if (g.sizePx.x > 0 && g.sizePx.y > 0)
            {
                float x0 = penX + g.bearingPx.x * scale;
                float y0 = penY - g.bearingPx.y * scale;
                float x1 = x0 + g.sizePx.x * scale;
                float y1 = y0 + g.sizePx.y * scale;

                glm::vec3 p0(x0, y0, 0.0f);
                glm::vec3 p1(x1, y0, 0.0f);
                glm::vec3 p2(x1, y1, 0.0f);
                glm::vec3 p3(x0, y1, 0.0f);

                glm::vec2 uv0 = g.uv0;
                glm::vec2 uv2 = g.uv1;
                glm::vec2 uv1(uv2.x, uv0.y);
                glm::vec2 uv3(uv0.x, uv2.y);

                DrawQuadRaw_Text(p0, p1, p2, p3, uv0, uv1, uv2, uv3, font.atlas, color);
            }

            penX += g.advancePx * scale;
        }
    }

    void VulkanUIRenderer::DrawQuadRaw_Text(const glm::vec3& p0, const glm::vec3& p1, const glm::vec3& p2, const glm::vec3& p3,
        const glm::vec2& uv0, const glm::vec2& uv1, const glm::vec2& uv2, const glm::vec2& uv3,
        const Ref<VulkanTexture>& texture, const glm::vec4& tintColor)
    {
        const uint32_t maxIndices = m_cfg.maxQuads * 6;
        if (m_textIndexCount + 6 > maxIndices)
            return;

        const uint32_t texSlot = AcquireTextTextureSlot(texture);

        VulkanUIGraphicsPipeline::VulkanUIQuadVertex* v = m_textPtr;

        v[0] = { p0, tintColor, uv0, texSlot };
        v[1] = { p1, tintColor, uv1, texSlot };
        v[2] = { p2, tintColor, uv2, texSlot };
        v[3] = { p3, tintColor, uv3, texSlot };

        m_textPtr += 4;
        m_textIndexCount += 6;
    }

    void VulkanUIRenderer::UploadTextVB()
    {
        const uint8_t* base = reinterpret_cast<const uint8_t*>(m_textCPU.data());
        const uint8_t* ptr = reinterpret_cast<const uint8_t*>(m_textPtr);
        const VkDeviceSize dataSize = VkDeviceSize(ptr - base);
        if (dataSize == 0) return;

        m_textVB->SetData(m_textCPU.data(), (size_t)dataSize);
    }

    void VulkanUIRenderer::UpdateTextDescriptors()
    {
        VulkanUITextGraphicsPipeline::CameraUBO cam{};
        cam.ViewProjection = MakeUIVP(m_viewportPx);
        m_textPipeline->UpdateCameraUBO(m_frameIndex, cam);

        std::vector<Ref<VulkanTexture>> full = m_textTextureSlots;
        m_textPipeline->UpdateUITextures(m_frameIndex, full);
    }

    void VulkanUIRenderer::BindAndDrawText(VkCommandBuffer cmd)
    {
        VkPipeline pipe = m_textPipeline->GetPipeline();
        VkPipelineLayout layout = m_textPipeline->GetPipelineLayout();
        vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipe);

        VulkanUIGraphicsPipeline::UIPushConstants pc{};
        pc.u_GlobalAlpha = 1.0f;
        pc.u_RoundRadiusPx = 0.0f;
        pc.u_RoundFeatherPx = 1.0f;
        pc._pad0 = 0.0f;
        pc.u_ClipRectPx = glm::vec4(0, 0, 0, 0);

        vkCmdPushConstants(cmd, layout, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
            0, sizeof(VulkanUIGraphicsPipeline::UIPushConstants), &pc);

        VkViewport vp{};
        vp.x = 0; vp.y = 0;
        vp.width = m_viewportPx.x;
        vp.height = m_viewportPx.y;
        vp.minDepth = 0.0f;
        vp.maxDepth = 1.0f;
        vkCmdSetViewport(cmd, 0, 1, &vp);

        VkRect2D sc{};
        sc.offset = { 0, 0 };
        sc.extent = { (uint32_t)m_viewportPx.x, (uint32_t)m_viewportPx.y };
        vkCmdSetScissor(cmd, 0, 1, &sc);

        VkDescriptorSet set0 = m_textPipeline->GetCameraDescriptorSet(m_frameIndex);
        VkDescriptorSet set1 = m_textPipeline->GetUITextureDescriptorSet(m_frameIndex);
        VkDescriptorSet sets[] = { set0, set1 };
        vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, layout, 0, 2, sets, 0, nullptr);

        VkBuffer vb = m_textVB->GetBuffer();
        VkDeviceSize offs = 0;
        vkCmdBindVertexBuffers(cmd, 0, 1, &vb, &offs);
        vkCmdBindIndexBuffer(cmd, m_indexBuffer->GetBuffer(), 0, VK_INDEX_TYPE_UINT32);

        vkCmdDrawIndexed(cmd, m_textIndexCount, 1, 0, 0, 0);
    }
}
