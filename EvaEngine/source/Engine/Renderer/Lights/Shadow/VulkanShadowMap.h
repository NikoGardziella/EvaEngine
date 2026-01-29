// VulkanShadowMap.h
#pragma once

#include "Engine/Core/Core.h"
#include <vulkan/vulkan.h>
#include <glm/glm.hpp>
#include "VulkanShadowGraphicsPipeline.h"

namespace Engine {

    class VulkanShadowMap
    {
    public:
        VulkanShadowMap() = default;
        ~VulkanShadowMap() = default;

        void InitShadowMap(VkDevice device, VkPhysicalDevice physicalDevice, uint32_t shadowMapSize = 2048);
        void CleanupShadowMap(VkDevice device);

        // Update light space matrix based on directional light
        void UpdateLightSpaceMatrix(const glm::vec3& lightDirection,
            const glm::vec3& sceneCenter,
            float sceneRadius);

        // Getters
        VkImage GetShadowMapImage() const { return m_shadowMapImage; }
        VkImageView GetShadowMapView() const { return m_shadowMapView; }
        VkSampler GetShadowMapSampler() const { return m_shadowMapSampler; }
        VkFramebuffer GetShadowFramebuffer() const { return m_shadowFramebuffer; }
        VkRenderPass GetShadowRenderPass() const { return m_shadowRenderPass; }
        const glm::mat4& GetLightSpaceMatrix() const { return m_lightSpaceMatrix; }
        uint32_t GetShadowMapSize() const { return m_shadowMapSize; }

        Ref<VulkanShadowGraphicsPipeline> GetShadowPipeline() { return m_shadowPipeline; };

    private:

        Ref<VulkanShadowGraphicsPipeline> m_shadowPipeline;



        VkDevice m_device = VK_NULL_HANDLE;
        uint32_t m_shadowMapSize = 2048;

        VkImage m_shadowMapImage = VK_NULL_HANDLE;
        VkDeviceMemory m_shadowMapMemory = VK_NULL_HANDLE;
        VkImageView m_shadowMapView = VK_NULL_HANDLE;
        VkSampler m_shadowMapSampler = VK_NULL_HANDLE;
        VkFramebuffer m_shadowFramebuffer = VK_NULL_HANDLE;
        VkRenderPass m_shadowRenderPass = VK_NULL_HANDLE;

        glm::mat4 m_lightSpaceMatrix = glm::mat4(1.0f);
    };

} 