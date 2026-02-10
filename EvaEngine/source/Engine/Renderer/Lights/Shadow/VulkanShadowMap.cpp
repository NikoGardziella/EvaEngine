#include "pch.h"
#include "VulkanShadowMap.h"

#include "Engine/Platform/Vulkan/VulkanContext.h"

namespace Engine {

    void VulkanShadowMap::UpdateLightSpaceMatrix(const glm::vec3& lightDir, const glm::vec3& cameraPos, float radius)
    {
        glm::vec3 dir = glm::normalize(lightDir);
        glm::vec3 lightPos = cameraPos - dir * 50.0f;

        glm::vec3 up = glm::vec3(0, 1, 0);
        if (std::abs(glm::dot(dir, up)) > 0.99f) up = glm::vec3(0, 0, 1);

        glm::mat4 lightView = glm::lookAtRH(lightPos, cameraPos, up);

        float ortho = radius;
        glm::mat4 lightProj = glm::orthoRH_ZO(-ortho, ortho, -ortho, ortho, 0.0f, 100.0f);

        m_lightSpaceMatrix = lightProj * lightView;
    }

    void VulkanShadowMap::UpdateTileShadowMatrix(const glm::vec3& lightDir, const glm::vec3& center, float radius) {
        glm::vec3 dir = glm::normalize(lightDir);
        glm::vec3 lightPos = center - dir * 4000.0f;

        glm::vec3 up = (std::abs(dir.z) > 0.99f) ? glm::vec3(0, 1, 0) : glm::vec3(0, 0, 1);
        glm::mat4 lightView = glm::lookAtRH(lightPos, center, up);

        glm::mat4 lightProj = glm::orthoRH_ZO(-radius, radius, -radius, radius, 1.0f, 8000.0f);

        m_lightSpaceMatrix = lightProj * lightView;
    }

 
    void VulkanShadowMap::CreateShadowTarget(VkDevice device, uint32_t size, ShadowTarget& target)
    {
        // 1. Image
        VkImageCreateInfo imageInfo{};
        imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        imageInfo.imageType = VK_IMAGE_TYPE_2D;
        imageInfo.format = VK_FORMAT_R32G32_SFLOAT;
        imageInfo.extent = { size, size, 1 };
        imageInfo.mipLevels = 1;
        imageInfo.arrayLayers = 1;
        imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
        imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
        imageInfo.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
        imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        vkCreateImage(device, &imageInfo, nullptr, &target.image);

        // 2. Memory
        VkMemoryRequirements memReqs;
        vkGetImageMemoryRequirements(device, target.image, &memReqs);

        VkMemoryAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        allocInfo.allocationSize = memReqs.size;
        allocInfo.memoryTypeIndex = VulkanContext::Get()->FindMemoryType(
            memReqs.memoryTypeBits,
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
        );
        vkAllocateMemory(device, &allocInfo, nullptr, &target.memory);
        vkBindImageMemory(device, target.image, target.memory, 0);

        // 3. Image view
        VkImageViewCreateInfo viewInfo{};
        viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        viewInfo.image = target.image;
        viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        viewInfo.format = VK_FORMAT_R32G32_SFLOAT;
        viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        viewInfo.subresourceRange.baseMipLevel = 0;
        viewInfo.subresourceRange.levelCount = 1;
        viewInfo.subresourceRange.baseArrayLayer = 0;
        viewInfo.subresourceRange.layerCount = 1;
        vkCreateImageView(device, &viewInfo, nullptr, &target.view);

        // 4. Sampler
        VkSamplerCreateInfo samplerInfo{};
        samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
        samplerInfo.magFilter = VK_FILTER_LINEAR;
        samplerInfo.minFilter = VK_FILTER_LINEAR;
        samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
        samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
        samplerInfo.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_BLACK;
        samplerInfo.compareEnable = VK_FALSE;
        vkCreateSampler(device, &samplerInfo, nullptr, &target.sampler);

        // 5. Render pass
        VkAttachmentDescription attachment{};
        attachment.format = VK_FORMAT_R32G32_SFLOAT;
        attachment.samples = VK_SAMPLE_COUNT_1_BIT;
        attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        attachment.finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

        VkAttachmentReference colorRef{};
        colorRef.attachment = 0;
        colorRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

        VkSubpassDescription subpass{};
        subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
        subpass.colorAttachmentCount = 1;
        subpass.pColorAttachments = &colorRef;
        subpass.pDepthStencilAttachment = nullptr;

        VkSubpassDependency dep{};
        dep.srcSubpass = 0;
        dep.dstSubpass = VK_SUBPASS_EXTERNAL;
        dep.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        dep.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
        dep.dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
        dep.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

        VkRenderPassCreateInfo renderPassInfo{};
        renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
        renderPassInfo.attachmentCount = 1;
        renderPassInfo.pAttachments = &attachment;
        renderPassInfo.subpassCount = 1;
        renderPassInfo.pSubpasses = &subpass;
        renderPassInfo.dependencyCount = 1;
        renderPassInfo.pDependencies = &dep;
        vkCreateRenderPass(device, &renderPassInfo, nullptr, &target.renderPass);

        // 6. Framebuffer
        VkFramebufferCreateInfo fbInfo{};
        fbInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        fbInfo.renderPass = target.renderPass;
        fbInfo.attachmentCount = 1;
        fbInfo.pAttachments = &target.view;
        fbInfo.width = size;
        fbInfo.height = size;
        fbInfo.layers = 1;
        vkCreateFramebuffer(device, &fbInfo, nullptr, &target.framebuffer);
    }

    void VulkanShadowMap::DestroyShadowTarget(VkDevice device, ShadowTarget& target)
    {
        if (target.framebuffer) vkDestroyFramebuffer(device, target.framebuffer, nullptr);
        if (target.renderPass)  vkDestroyRenderPass(device, target.renderPass, nullptr);
        if (target.sampler)     vkDestroySampler(device, target.sampler, nullptr);
        if (target.view)        vkDestroyImageView(device, target.view, nullptr);
        if (target.image)       vkDestroyImage(device, target.image, nullptr);
        if (target.memory)      vkFreeMemory(device, target.memory, nullptr);
        target = {};
    }


    void VulkanShadowMap::InitShadowMap(VkDevice device, VkPhysicalDevice physicalDevice, uint32_t shadowMapSize)
    {
        m_device = device;

        CreateShadowTarget(device, shadowMapSize, m_3dShadow);
        CreateShadowTarget(device, shadowMapSize, m_tileShadow);

        EE_CORE_INFO("Shadow maps initialized: {}x{} (3D + Tile)", shadowMapSize, shadowMapSize);

        m_shadowPipeline = std::make_shared<VulkanShadowGraphicsPipeline>();
        m_shadowPipeline->Init(m_device, m_3dShadow.renderPass);  // both render passes are identical
    
        
        
    }

    void VulkanShadowMap::CleanupShadowMap(VkDevice device)
    {
        DestroyShadowTarget(device, m_3dShadow);
        DestroyShadowTarget(device, m_tileShadow);
    }

    void VulkanShadowMap::TransitionToReadable(VkCommandBuffer cmd, ShadowTarget& target)
    {
        VkImageMemoryBarrier barrier{};
        barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;  // don't care about contents
        barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.image = target.image;
        barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        barrier.subresourceRange.baseMipLevel = 0;
        barrier.subresourceRange.levelCount = 1;
        barrier.subresourceRange.baseArrayLayer = 0;
        barrier.subresourceRange.layerCount = 1;
        barrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

        vkCmdPipelineBarrier(cmd,
            VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
            VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
            0, 0, nullptr, 0, nullptr, 1, &barrier);
    }
}