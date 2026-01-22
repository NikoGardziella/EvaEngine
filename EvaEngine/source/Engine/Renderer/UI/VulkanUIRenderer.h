#pragma once
#include "VulkanUIGraphicsPipeline.h"
#include <glm/glm.hpp>
#include <Engine/UI/Font.h>
#include <Engine/UI/UITransform2D.h>
#include <Engine/Core/Core.h>

#include <vector>
#include <unordered_map>
#include <string_view>
#include <cstdint>
#include <vulkan/vulkan_core.h>

namespace Engine
{
    class VulkanContext;
    class VulkanTexture;
    class VulkanBuffer;
    class VulkanIndexBuffer;

    class VulkanUITextGraphicsPipeline;   // R8 text coverage


    struct UIRendererInitConfig
    {
        uint32_t maxQuads = 2000;
        uint32_t maxTextures = 64;
    };

    class VulkanUIRenderer
    {
    public:
        static VulkanUIRenderer* s_active;

        VulkanUIRenderer();
        VulkanUIRenderer(VulkanContext* vulkanContext);
        ~VulkanUIRenderer();

        void Init(const UIRendererInitConfig& cfg, VulkanContext* vulkanContext);
        void Shutdown();

        // Called once per frame
        void Begin(uint32_t frameIndex, glm::vec2 viewportPx);
        void EndAndRecord(VkCommandBuffer cmd);

        // Static API (callable from Scene/UI elements)
        static void BeginFrame(VulkanUIRenderer& r, uint32_t frameIndex, glm::vec2 viewportPx)
        {
            s_active = &r;
            s_active->Begin(frameIndex, viewportPx);
        }

        static void EndFrame(VkCommandBuffer cmd)
        {
            if (s_active) s_active->EndAndRecord(cmd);
        }

        static void DrawUIIcon(const Ref<VulkanTexture>& icon, const UITransform2D& tr, glm::vec4 tint)
        {
            if (s_active) s_active->DrawUIIcon_Impl(icon, tr, tint);
        }

        static void DrawUIText(const Ref<Font>& font, std::string_view text,
            const UITransform2D& tr, glm::vec4 color, float scale = 1.0f)
        {
            if (s_active) s_active->DrawText_Impl(font, text, tr, color, scale);
        }

        glm::vec2 GetViewportPx() const { return m_viewportPx; }

    private:

        //  Common 
        glm::mat4 MakeUIVP(glm::vec2 viewportPx) const;

        //  Icon batch 
        void ResetIcons();
        uint32_t AcquireIconTextureSlot(const Ref<VulkanTexture>& tex);
        void DrawUIIcon_Impl(const Ref<VulkanTexture>& icon, const UITransform2D& tr, glm::vec4 tint);
        void DrawQuadRaw_Icons(const glm::vec3& p0, const glm::vec3& p1, const glm::vec3& p2, const glm::vec3& p3,
            const glm::vec2& uv0, const glm::vec2& uv1, const glm::vec2& uv2, const glm::vec2& uv3,
            const Ref<VulkanTexture>& texture, const glm::vec4& tintColor);
        void UploadIconsVB();
        void UpdateIconsDescriptors();
        void BindAndDrawIcons(VkCommandBuffer cmd);

        //  Text batch 
        void ResetText();
        uint32_t AcquireTextTextureSlot(const Ref<VulkanTexture>& tex);
        void DrawText_Impl(const Ref<Font>& fontRef, std::string_view text,
            const UITransform2D& t, glm::vec4 color, float scale);
        void DrawQuadRaw_Text(const glm::vec3& p0, const glm::vec3& p1, const glm::vec3& p2, const glm::vec3& p3,
            const glm::vec2& uv0, const glm::vec2& uv1, const glm::vec2& uv2, const glm::vec2& uv3,
            const Ref<VulkanTexture>& texture, const glm::vec4& tintColor);
        void UploadTextVB();
        void UpdateTextDescriptors();
        void BindAndDrawText(VkCommandBuffer cmd);

    private:
        VulkanContext* m_vulkanContext = nullptr;
        VkDevice m_device = nullptr;

        UIRendererInitConfig m_cfg{};
        bool m_initialized = false;

        uint32_t m_frameIndex = 0;
        glm::vec2 m_viewportPx{ 0, 0 };

        // Pipelines
        Ref<VulkanUIGraphicsPipeline>     m_iconPipeline; // RGBA icons
        Ref<VulkanUITextGraphicsPipeline> m_textPipeline; // R8 text

        // Fallback textures
        Ref<VulkanTexture> m_fallbackWhiteRGBA; // icons fallback (RGBA8 UNORM)
        Ref<VulkanTexture> m_fallbackWhiteR8;   // text fallback (R8 UNORM, value=255)

        // Index buffer shared
        Ref<VulkanIndexBuffer> m_indexBuffer;

        //  ICONS 
        std::vector<VulkanUIGraphicsPipeline::VulkanUIQuadVertex> m_iconCPU;
        VulkanUIGraphicsPipeline::VulkanUIQuadVertex* m_iconPtr = nullptr;
        uint32_t m_iconIndexCount = 0;

        Ref<VulkanBuffer> m_iconVB;

        std::vector<Ref<VulkanTexture>> m_iconTextureSlots;
        std::unordered_map<uint64_t, uint32_t> m_iconTexLUT;
        uint32_t m_iconTextureSlotCount = 0;

        //  TEXT 
        std::vector<VulkanUIGraphicsPipeline::VulkanUIQuadVertex> m_textCPU;
        VulkanUIGraphicsPipeline::VulkanUIQuadVertex* m_textPtr = nullptr;
        uint32_t m_textIndexCount = 0;

        Ref<VulkanBuffer> m_textVB;

        std::vector<Ref<VulkanTexture>> m_textTextureSlots;
        std::unordered_map<uint64_t, uint32_t> m_textTexLUT;
        uint32_t m_textTextureSlotCount = 0;
    };
}
