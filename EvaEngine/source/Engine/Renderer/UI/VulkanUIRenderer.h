#pragma once

#include <vulkan/vulkan.h>
#include <glm/glm.hpp>
#include <memory>
#include <unordered_map>
#include <vector>
#include <array>
#include <cstdint>
#include "VulkanUIGraphicsPipeline.h"
#include <Engine/Platform/Vulkan/VulkanContext.h>
#include <Engine/UI/UITransform2D.h>

namespace Engine
{
    class VulkanTexture;
    class VulkanBuffer;
    class VulkanUIGraphicsPipeline;




    class VulkanUIRenderer
    {
    public:
        struct UIRendererInitConfig
        {
            uint32_t maxQuads = 20000;
            uint32_t maxTextures = MAX_UI_TEXTURES;
        };


        static void BeginFrame(VulkanUIRenderer& r, uint32_t frame, glm::vec2 viewportPx)
        {
            s_active = &r;
            r.Begin(frame, viewportPx);
        }

        static void EndFrame(VkCommandBuffer cmd)
        {
            EE_CORE_ASSERT(s_active, "UIRenderer not active");
            s_active->EndAndRecord(cmd);
            s_active = nullptr;
        }

        static void DrawUIIcon(const Ref<VulkanTexture>& icon, UITransform2D tr, glm::vec4 tint)
        {
            EE_CORE_ASSERT(s_active, "UIRenderer not active");

            s_active->DrawUIIcon_Impl(icon, tr, tint);
        }




        VulkanUIRenderer() = default;
        VulkanUIRenderer(VulkanContext* m_vulkanContext);

        ~VulkanUIRenderer();

        VulkanUIRenderer(const VulkanUIRenderer&) = delete;
        VulkanUIRenderer& operator=(const VulkanUIRenderer&) = delete;

        void Init(const UIRendererInitConfig& cfg, VulkanContext* vulkanContext);

    private:
        void Shutdown();

        void Begin(uint32_t frameIndex, glm::vec2 viewportPx);
        void EndAndRecord(VkCommandBuffer cmd); 


        void DrawUIIcon_Impl(const Ref<VulkanTexture>& icon, UITransform2D tr, glm::vec4 tint);

        void DrawQuadRaw(const glm::vec3& p0, const glm::vec3& p1, const glm::vec3& p2, const glm::vec3& p3,
            const glm::vec2& uv0, const glm::vec2& uv1, const glm::vec2& uv2, const glm::vec2& uv3,
            const Ref<VulkanTexture>& texture, const glm::vec4& tintColor);


    private:
        void ResetBatch();
        void UploadVB();
        void UpdateDescriptors();
        void BindAndDraw(VkCommandBuffer cmd);

        uint32_t AcquireTextureSlot(const Ref<VulkanTexture>& texture);

        glm::mat4 MakeUIVP(glm::vec2 viewportPx) const;

    private:

        static VulkanUIRenderer* s_active;

        VkDevice m_device = VK_NULL_HANDLE;

        Ref<VulkanTexture> m_fallbackWhite;

        UIRendererInitConfig m_cfg{};
        bool m_initialized = false;

        // Per-frame data
        uint32_t m_frameIndex = 0;
        glm::vec2 m_viewportPx{ 1.0f, 1.0f };
        
        std::vector<VulkanUIGraphicsPipeline::VulkanUIQuadVertex> m_cpuVertices;
        VulkanUIGraphicsPipeline::VulkanUIQuadVertex* m_vtxPtr = nullptr;

        uint32_t m_indexCount = 0;

        // GPU buffers
        Ref<VulkanBuffer> m_vertexBuffer;
        Ref<VulkanIndexBuffer> m_indexBuffer;

        // Texture slots for this batch (maps to pipeline's set1 texture array)
        std::vector<Ref<VulkanTexture>> m_textureSlots;
        uint32_t m_textureSlotCount = 0;

        std::unordered_map<uint64_t, uint32_t> m_texLUT;


        Ref<VulkanUIGraphicsPipeline> m_uIGraphicsPipeline;
        VulkanContext* m_vulkanContext;

    };
}
