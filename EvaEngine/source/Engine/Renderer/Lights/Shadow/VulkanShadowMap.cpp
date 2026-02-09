#include "pch.h"
#include "VulkanShadowMap.h"

#include "Engine/Platform/Vulkan/VulkanContext.h"

namespace Engine {

    void VulkanShadowMap::UpdateLightSpaceMatrix(const glm::vec3& lightDir, const glm::vec3& cameraPos, float radius)
    {
        glm::vec3 dir = glm::normalize(lightDir);

        // Position the light "behind" the camera relative to light direction
        // 100.0f is fine, but ensure the Z-range in ortho covers the scene
        glm::vec3 lightPos = cameraPos - dir * 50.0f;

        glm::vec3 up = glm::vec3(0, 1, 0);
        if (std::abs(glm::dot(dir, up)) > 0.99f) up = glm::vec3(0, 0, 1);

        glm::mat4 lightView = glm::lookAtRH(lightPos, cameraPos, up);

        // Keep the near/far planes tight around the radius to maximize depth precision
        float ortho = radius;
        glm::mat4 lightProj = glm::orthoRH_ZO(-ortho, ortho, -ortho, ortho, 0.0f, 100.0f);

        m_lightSpaceMatrix = lightProj * lightView;
    }


    // This matrix is specifically for the 2D Pixel World
    void VulkanShadowMap::UpdateTileShadowMatrix(const glm::vec3& lightDir, const glm::vec3& center, float radius) {
        glm::vec3 dir = glm::normalize(lightDir);
        // Move light 4000 pixels away
        glm::vec3 lightPos = center - dir * 4000.0f;

        glm::vec3 up = (std::abs(dir.z) > 0.99f) ? glm::vec3(0, 1, 0) : glm::vec3(0, 0, 1);
        glm::mat4 lightView = glm::lookAtRH(lightPos, center, up);

        // Ortho view that matches the sceneRadius
        // Near: 1.0, Far: 8000.0 to ensure the 4000-unit distance is covered
        glm::mat4 lightProj = glm::orthoRH_ZO(-radius, radius, -radius, radius, 1.0f, 8000.0f);

        m_lightSpaceMatrix = lightProj * lightView;
    }


    void VulkanShadowMap::InitShadowMap(VkDevice device, VkPhysicalDevice physicalDevice, uint32_t shadowMapSize)
    {
        m_device = device;
        VulkanContext* vulkanContext = VulkanContext::Get();
    

        

        // 1. Create depth image
        VkImageCreateInfo imageInfo{};
        imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        imageInfo.imageType = VK_IMAGE_TYPE_2D;
        imageInfo.format = VK_FORMAT_D32_SFLOAT;  // 32-bit depth
        imageInfo.extent = { shadowMapSize, shadowMapSize, 1 };
        imageInfo.mipLevels = 1;
        imageInfo.arrayLayers = 1;
        imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
        imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
        imageInfo.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
        imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

        vkCreateImage(device, &imageInfo, nullptr, &m_shadowMapImage);

        // 2. Allocate memory
        VkMemoryRequirements memReqs;
        vkGetImageMemoryRequirements(device, m_shadowMapImage, &memReqs);

        VkMemoryAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        allocInfo.allocationSize = memReqs.size;
        allocInfo.memoryTypeIndex = VulkanContext::Get()->FindMemoryType(
            memReqs.memoryTypeBits,
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
        );

        vkAllocateMemory(device, &allocInfo, nullptr, &m_shadowMapMemory);
        vkBindImageMemory(device, m_shadowMapImage, m_shadowMapMemory, 0);

        // 3. Create image view
        VkImageViewCreateInfo viewInfo{};
        viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        viewInfo.image = m_shadowMapImage;
        viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        viewInfo.format = VK_FORMAT_D32_SFLOAT;
        viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
        viewInfo.subresourceRange.baseMipLevel = 0;
        viewInfo.subresourceRange.levelCount = 1;
        viewInfo.subresourceRange.baseArrayLayer = 0;
        viewInfo.subresourceRange.layerCount = 1;

        vkCreateImageView(device, &viewInfo, nullptr, &m_shadowMapView);

        // 4. Create sampler
        VkSamplerCreateInfo samplerInfo{};
        samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
        samplerInfo.magFilter = VK_FILTER_LINEAR;
        samplerInfo.minFilter = VK_FILTER_LINEAR;
        samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
        samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
        samplerInfo.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_BLACK;
        samplerInfo.compareEnable = VK_FALSE;  // CHANGED: Disable for debugging
        // samplerInfo.compareOp = VK_COMPARE_OP_LESS;  // Comment out

        vkCreateSampler(device, &samplerInfo, nullptr, &m_shadowMapSampler);

        // 5. Create shadow render pass
        VkAttachmentDescription depthAttachment{};
        depthAttachment.format = VK_FORMAT_D32_SFLOAT;
        depthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
        depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;


        VkSubpassDependency dep{};
        dep.srcSubpass = 0;
        dep.dstSubpass = VK_SUBPASS_EXTERNAL;

        // after depth writes
        dep.srcStageMask = VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
        dep.srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

        // before fragment sampling
        dep.dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
        dep.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;


        VkAttachmentReference depthRef{};
        depthRef.attachment = 0;
        depthRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

        VkSubpassDescription subpass{};
        subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
        subpass.pDepthStencilAttachment = &depthRef;





        VkRenderPassCreateInfo renderPassInfo{};
        renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
        renderPassInfo.attachmentCount = 1;
        renderPassInfo.pAttachments = &depthAttachment;
        renderPassInfo.subpassCount = 1;
        renderPassInfo.pSubpasses = &subpass;
        renderPassInfo.dependencyCount = 1;
        renderPassInfo.pDependencies = &dep;
        vkCreateRenderPass(device, &renderPassInfo, nullptr, &m_shadowRenderPass);

        // 6. Create framebuffer
        VkFramebufferCreateInfo fbInfo{};
        fbInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        fbInfo.renderPass = m_shadowRenderPass;
        fbInfo.attachmentCount = 1;
        fbInfo.pAttachments = &m_shadowMapView;
        fbInfo.width = shadowMapSize;
        fbInfo.height = shadowMapSize;
        fbInfo.layers = 1;

        vkCreateFramebuffer(device, &fbInfo, nullptr, &m_shadowFramebuffer);

        EE_CORE_INFO("Shadow map initialized: {}x{}", shadowMapSize, shadowMapSize);

        m_shadowPipeline = std::make_shared<VulkanShadowGraphicsPipeline>();
        m_shadowPipeline->Init(m_device, m_shadowRenderPass);

    }

    void VulkanShadowMap::CleanupShadowMap(VkDevice device)
    {
        if (m_shadowFramebuffer) vkDestroyFramebuffer(device, m_shadowFramebuffer, nullptr);
        if (m_shadowRenderPass) vkDestroyRenderPass(device, m_shadowRenderPass, nullptr);
        if (m_shadowMapSampler) vkDestroySampler(device, m_shadowMapSampler, nullptr);
        if (m_shadowMapView) vkDestroyImageView(device, m_shadowMapView, nullptr);
        if (m_shadowMapImage) vkDestroyImage(device, m_shadowMapImage, nullptr);
        if (m_shadowMapMemory) vkFreeMemory(device, m_shadowMapMemory, nullptr);
    }

}