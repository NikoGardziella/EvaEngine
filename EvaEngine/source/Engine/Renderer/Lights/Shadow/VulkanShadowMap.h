// VulkanShadowMap.h
#pragma once

#include "Engine/Core/Core.h"
#include <vulkan/vulkan.h>
#include <glm/glm.hpp>
#include "VulkanShadowGraphicsPipeline.h"

namespace Engine {

    class VulkanShadowMap
    {
    private:
        struct ShadowTarget {
            VkImage        image = VK_NULL_HANDLE;
            VkDeviceMemory memory = VK_NULL_HANDLE;
            VkImageView    view = VK_NULL_HANDLE;
            VkSampler      sampler = VK_NULL_HANDLE;
            VkRenderPass   renderPass = VK_NULL_HANDLE;
            VkFramebuffer  framebuffer = VK_NULL_HANDLE;
            // Depth (optional)
            VkImage        depthImage = VK_NULL_HANDLE;
            VkDeviceMemory depthMemory = VK_NULL_HANDLE;
            VkImageView    depthView = VK_NULL_HANDLE;
        };



    public:
        VulkanShadowMap() = default;
        ~VulkanShadowMap() = default;


        void InitShadowMap(VkDevice device, VkPhysicalDevice physicalDevice, uint32_t shadowMapSize = 2048);
        void CleanupShadowMap(VkDevice device);

        void TransitionToReadable(VkCommandBuffer cmd, ShadowTarget& target);

        // Update light space matrix based on directional light
        void UpdateLightSpaceMatrix(const glm::vec3& lightDirection, const glm::vec3& sceneCenter,  float sceneRadius);

        void UpdateTileShadowMatrix(const glm::vec3& lightDir, const glm::vec3& center, float radius);
        void CreateShadowTarget(VkDevice device, uint32_t size, ShadowTarget& target, bool needsDepth);
        void CreateShadowTarget(VkDevice device, uint32_t size, ShadowTarget& target);
        void DestroyShadowTarget(VkDevice device, ShadowTarget& target);
        // Getters
     
        const glm::mat4& GetLightSpaceMatrix() const { return m_lightSpaceMatrix; }
        const glm::vec3& GetLightDirection() const { return m_lightDirection; }
        void SetLightDirection(glm::vec3 lightDirection) {  m_lightDirection = lightDirection; }
        uint32_t GetShadowMapSize() const { return m_shadowMapSize; }

        Ref<VulkanShadowGraphicsPipeline> GetShadowPipeline() { return m_shadowPipeline; };

        ShadowTarget Get3DShadowmap() const { return m_3dShadow; }
        ShadowTarget GetTileShadowmap() const { return m_tileShadow; }

    private:

        Ref<VulkanShadowGraphicsPipeline> m_shadowPipeline;
        ShadowTarget m_3dShadow;
        ShadowTarget m_tileShadow;



        VkDevice m_device = VK_NULL_HANDLE;
        uint32_t m_shadowMapSize = 2048;


        glm::mat4 m_lightSpaceMatrix = glm::mat4(1.0f);

        glm::vec3 m_lightDirection = glm::vec3(1.0f);
    };

} 