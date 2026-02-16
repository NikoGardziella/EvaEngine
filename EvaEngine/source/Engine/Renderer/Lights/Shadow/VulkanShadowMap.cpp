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

    void VulkanShadowMap::CreateShadowTarget(VkDevice device, uint32_t size, ShadowTarget& target, bool needsDepth)
    {
        // 1. Color image (R32G32 for depth + worldY)
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

        VkMemoryRequirements memReqs;
        vkGetImageMemoryRequirements(device, target.image, &memReqs);

        VkMemoryAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        allocInfo.allocationSize = memReqs.size;
        allocInfo.memoryTypeIndex = VulkanContext::Get()->FindMemoryType(
            memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
        vkAllocateMemory(device, &allocInfo, nullptr, &target.memory);
        vkBindImageMemory(device, target.image, target.memory, 0);

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

        // 2. Sampler
        VkSamplerCreateInfo samplerInfo{};
        samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
        samplerInfo.magFilter = VK_FILTER_LINEAR;
        samplerInfo.minFilter = VK_FILTER_LINEAR;
        samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
        samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
        samplerInfo.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_BLACK;
        samplerInfo.compareEnable = VK_FALSE;
        vkCreateSampler(device, &samplerInfo, nullptr, &target.sampler);

        // 3. Depth image (optional)
        if (needsDepth)
        {
            VkImageCreateInfo depthImageInfo{};
            depthImageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
            depthImageInfo.imageType = VK_IMAGE_TYPE_2D;
            depthImageInfo.format = VK_FORMAT_D32_SFLOAT;
            depthImageInfo.extent = { size, size, 1 };
            depthImageInfo.mipLevels = 1;
            depthImageInfo.arrayLayers = 1;
            depthImageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
            depthImageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
            depthImageInfo.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
            depthImageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
            vkCreateImage(device, &depthImageInfo, nullptr, &target.depthImage);

            VkMemoryRequirements depthMemReqs;
            vkGetImageMemoryRequirements(device, target.depthImage, &depthMemReqs);

            VkMemoryAllocateInfo depthAllocInfo{};
            depthAllocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
            depthAllocInfo.allocationSize = depthMemReqs.size;
            depthAllocInfo.memoryTypeIndex = VulkanContext::Get()->FindMemoryType(
                depthMemReqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
            vkAllocateMemory(device, &depthAllocInfo, nullptr, &target.depthMemory);
            vkBindImageMemory(device, target.depthImage, target.depthMemory, 0);

            VkImageViewCreateInfo depthViewInfo{};
            depthViewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
            depthViewInfo.image = target.depthImage;
            depthViewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
            depthViewInfo.format = VK_FORMAT_D32_SFLOAT;
            depthViewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
            depthViewInfo.subresourceRange.baseMipLevel = 0;
            depthViewInfo.subresourceRange.levelCount = 1;
            depthViewInfo.subresourceRange.baseArrayLayer = 0;
            depthViewInfo.subresourceRange.layerCount = 1;
            vkCreateImageView(device, &depthViewInfo, nullptr, &target.depthView);
        }

        // 4. Render pass
        std::vector<VkAttachmentDescription> attachments;

        // Color attachment
        VkAttachmentDescription colorAttachment{};
        colorAttachment.format = VK_FORMAT_R32G32_SFLOAT;
        colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
        colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        colorAttachment.finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        attachments.push_back(colorAttachment);

        VkAttachmentReference colorRef{};
        colorRef.attachment = 0;
        colorRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

        VkAttachmentReference depthRef{};

        if (needsDepth)
        {
            VkAttachmentDescription depthAttachment{};
            depthAttachment.format = VK_FORMAT_D32_SFLOAT;
            depthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
            depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
            depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE; // we don't sample it
            depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
            depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
            depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
            depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
            attachments.push_back(depthAttachment);

            depthRef.attachment = 1;
            depthRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
        }

        VkSubpassDescription subpass{};
        subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
        subpass.colorAttachmentCount = 1;
        subpass.pColorAttachments = &colorRef;
        subpass.pDepthStencilAttachment = needsDepth ? &depthRef : nullptr;

        VkSubpassDependency dep{};
        dep.srcSubpass = 0;
        dep.dstSubpass = VK_SUBPASS_EXTERNAL;
        dep.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT |
            VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
        dep.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT |
            VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
        dep.dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
        dep.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

        VkRenderPassCreateInfo renderPassInfo{};
        renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
        renderPassInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
        renderPassInfo.pAttachments = attachments.data();
        renderPassInfo.subpassCount = 1;
        renderPassInfo.pSubpasses = &subpass;
        renderPassInfo.dependencyCount = 1;
        renderPassInfo.pDependencies = &dep;
        vkCreateRenderPass(device, &renderPassInfo, nullptr, &target.renderPass);

        // 5. Framebuffer
        std::vector<VkImageView> fbAttachments = { target.view };
        if (needsDepth)
            fbAttachments.push_back(target.depthView);

        VkFramebufferCreateInfo fbInfo{};
        fbInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        fbInfo.renderPass = target.renderPass;
        fbInfo.attachmentCount = static_cast<uint32_t>(fbAttachments.size());
        fbInfo.pAttachments = fbAttachments.data();
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
        if (target.depthView)   vkDestroyImageView(device, target.depthView, nullptr);
        if (target.depthImage)  vkDestroyImage(device, target.depthImage, nullptr);
        if (target.depthMemory) vkFreeMemory(device, target.depthMemory, nullptr);
        target = {};
    }


    void VulkanShadowMap::InitShadowMap(VkDevice device, VkPhysicalDevice physicalDevice, uint32_t shadowMapSize)
    {
        m_device = device;

        CreateShadowTarget(device, shadowMapSize, m_3dShadow, false);
        CreateShadowTarget(device, shadowMapSize, m_tileShadow, true);

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