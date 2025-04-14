#include "pch.h"
#include "VulkanSwapchain.h"
#include "VulkanUtils.h"

namespace Engine {

    VulkanSwapchain::VulkanSwapchain(VkDevice device, VkSurfaceKHR surface, VkPhysicalDevice physicalDevice)
        : m_device(device), m_surface(surface), m_physicalDevice(physicalDevice), m_swapchain(VK_NULL_HANDLE)
    {
        CreateSwapchain();
        CreateImageViews();
    }

    VulkanSwapchain::~VulkanSwapchain()
    {
        Cleanup();
    }

    void VulkanSwapchain::CreateSwapchain()
    {
        VulkanContext::SwapChainSupportDetails swapChainSupport = VulkanUtils::QuerySwapChainSupport(m_physicalDevice, m_surface);

        VkSurfaceFormatKHR surfaceFormat = ChooseSwapSurfaceFormat(swapChainSupport.formats);
        VkPresentModeKHR presentMode = ChooseSwapPresentMode(swapChainSupport.presentModes);
        VkExtent2D extent = ChooseSwapExtent(swapChainSupport.capabilities);

        uint32_t imageCount = swapChainSupport.capabilities.minImageCount + 1;
        if (swapChainSupport.capabilities.maxImageCount > 0 && imageCount > swapChainSupport.capabilities.maxImageCount)
        {
            imageCount = swapChainSupport.capabilities.maxImageCount;
        }

        VkSwapchainCreateInfoKHR createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
        createInfo.surface = m_surface;
        createInfo.minImageCount = imageCount;
        createInfo.imageFormat = surfaceFormat.format;
        createInfo.imageColorSpace = surfaceFormat.colorSpace;
        createInfo.imageExtent = extent;
        createInfo.imageArrayLayers = 1;
        createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;

        VulkanContext::QueueFamilyIndices indices = VulkanUtils::FindQueueFamilies(m_physicalDevice, m_surface);
        uint32_t queueFamilyIndices[] = { indices.graphicsFamily.value(), indices.presentFamily.value() };

        if (indices.graphicsFamily != indices.presentFamily) {
            createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
            createInfo.queueFamilyIndexCount = 2;
            createInfo.pQueueFamilyIndices = queueFamilyIndices;
        }
        else
        {
            createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
            createInfo.queueFamilyIndexCount = 0;
            createInfo.pQueueFamilyIndices = nullptr;
        }

        createInfo.preTransform = swapChainSupport.capabilities.currentTransform;
        createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
        createInfo.presentMode = presentMode;
        createInfo.clipped = VK_TRUE;
        createInfo.oldSwapchain = VK_NULL_HANDLE;

        if (vkCreateSwapchainKHR(m_device, &createInfo, nullptr, &m_swapchain) != VK_SUCCESS)
        {
            throw std::runtime_error("Failed to create Vulkan swap chain!");
        }

        vkGetSwapchainImagesKHR(m_device, m_swapchain, &imageCount, nullptr);
        m_swapchainImages.resize(imageCount);
        vkGetSwapchainImagesKHR(m_device, m_swapchain, &imageCount, m_swapchainImages.data());

        m_swapchainImageFormat = surfaceFormat.format;
        m_swapchainExtent = extent;

       
    }

    void VulkanSwapchain::CreateImageViews()
    {
        m_swapchainImageViews.resize(m_swapchainImages.size());
        const VkFormat swapchainFormat = m_swapchainImageFormat;
        const VkExtent2D extent = m_swapchainExtent;
        size_t imageCount = m_swapchainImages.size();

        for (size_t i = 0; i < imageCount; ++i)
        {
            VkImageViewCreateInfo viewCreateInfo{};
            viewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
            viewCreateInfo.image = m_swapchainImages[i];
            viewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
            viewCreateInfo.format = swapchainFormat;
            viewCreateInfo.components = { VK_COMPONENT_SWIZZLE_IDENTITY };
            viewCreateInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            viewCreateInfo.subresourceRange.baseMipLevel = 0;
            viewCreateInfo.subresourceRange.levelCount = 1;
            viewCreateInfo.subresourceRange.baseArrayLayer = 0;
            viewCreateInfo.subresourceRange.layerCount = 1;

            if (vkCreateImageView(m_device, &viewCreateInfo, nullptr, &m_swapchainImageViews[i]) != VK_SUCCESS)
            {
                EE_CORE_ERROR("Failed to create image views for swapchain images");
            }
            else
            {
                EE_CORE_INFO("Swapchain image view [{}] created", i);
            }
        }

        // === Game framebuffer color attachments ===
        m_gameColorAttachments.resize(imageCount);
        m_gameColorAttachmentMemories.resize(imageCount);
        m_gameColorAttachmentImageViews.resize(imageCount);

        for (size_t i = 0; i < imageCount; ++i)
        {
            // Create the image
            VkImageCreateInfo imageInfo{};
            imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
            imageInfo.imageType = VK_IMAGE_TYPE_2D;
            imageInfo.extent.width = extent.width;
            imageInfo.extent.height = extent.height;
            imageInfo.extent.depth = 1;
            imageInfo.mipLevels = 1;
            imageInfo.arrayLayers = 1;
            imageInfo.format = swapchainFormat;
            imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
            imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
            imageInfo.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
            imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
            imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

            if (vkCreateImage(m_device, &imageInfo, nullptr, &m_gameColorAttachments[i]) != VK_SUCCESS)
            {
                EE_CORE_ERROR("Failed to create game framebuffer color image [{}]", i);
            }

            VkMemoryRequirements memRequirements;
            vkGetImageMemoryRequirements(m_device, m_gameColorAttachments[i], &memRequirements);

            VkMemoryAllocateInfo allocInfo{};
            allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
            allocInfo.allocationSize = memRequirements.size;
            allocInfo.memoryTypeIndex = FindMemoryType(memRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

            if (vkAllocateMemory(m_device, &allocInfo, nullptr, &m_gameColorAttachmentMemories[i]) != VK_SUCCESS)
            {
                EE_CORE_ERROR("Failed to allocate memory for game framebuffer image [{}]", i);
            }

            vkBindImageMemory(m_device, m_gameColorAttachments[i], m_gameColorAttachmentMemories[i], 0);

            // Create the image view
            VkImageViewCreateInfo viewInfo{};
            viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
            viewInfo.image = m_gameColorAttachments[i];
            viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
            viewInfo.format = swapchainFormat;
            viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            viewInfo.subresourceRange.baseMipLevel = 0;
            viewInfo.subresourceRange.levelCount = 1;
            viewInfo.subresourceRange.baseArrayLayer = 0;
            viewInfo.subresourceRange.layerCount = 1;

            if (vkCreateImageView(m_device, &viewInfo, nullptr, &m_gameColorAttachmentImageViews[i]) != VK_SUCCESS)
            {
                EE_CORE_ERROR("Failed to create image view for game framebuffer [{}]", i);
            }
            else
            {
                EE_CORE_INFO("Game framebuffer image view [{}] created", i);
            }
        }

  
    }


    void VulkanSwapchain::CreateFramebuffers(VkRenderPass renderPass, VkRenderPass imGuiRenderPass, VkDevice device)
    {
        // Resize framebuffer vectors based on the number of swapchain images
        size_t swapchainImageCount = m_swapchainImageViews.size();
        m_swapchainFramebuffers.resize(swapchainImageCount);
        m_imguiFramebuffers.resize(swapchainImageCount);
        m_gameFramebuffers.resize(swapchainImageCount);

        // Create the scene framebuffers
        for (size_t i = 0; i < swapchainImageCount; ++i)
        {
            VkImageView swapchainImageView = m_swapchainImageViews[i];

            // === Scene framebuffer ===
            VkImageView sceneAttachments[] = { swapchainImageView };

            VkFramebufferCreateInfo sceneFramebufferInfo = {};
            sceneFramebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
            sceneFramebufferInfo.renderPass = renderPass; // Main scene render pass
            sceneFramebufferInfo.attachmentCount = static_cast<uint32_t>(std::size(sceneAttachments));
            sceneFramebufferInfo.pAttachments = sceneAttachments;
            sceneFramebufferInfo.width = m_swapchainExtent.width;
            sceneFramebufferInfo.height = m_swapchainExtent.height;
            sceneFramebufferInfo.layers = 1;

            VkFramebuffer framebuffer;
            if (vkCreateFramebuffer(device, &sceneFramebufferInfo, nullptr, &framebuffer) != VK_SUCCESS)
            {
                EE_CORE_ERROR("Failed to create scene framebuffer!");
            }
            else
            {
                m_swapchainFramebuffers[i] = framebuffer;
                EE_CORE_INFO("Scene framebuffer created");
            }
        }

        // Create the ImGui framebuffers
        for (size_t i = 0; i < swapchainImageCount; ++i)
        {
            VkImageView imguiImageView = m_swapchainImageViews[i]; // Same swapchain image

            // === ImGui framebuffer ===
            VkImageView imguiAttachments[] = { imguiImageView };

            VkFramebufferCreateInfo imguiFramebufferInfo = {};
            imguiFramebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
            imguiFramebufferInfo.renderPass = imGuiRenderPass; // ImGui render pass
            imguiFramebufferInfo.attachmentCount = static_cast<uint32_t>(std::size(imguiAttachments));
            imguiFramebufferInfo.pAttachments = imguiAttachments;
            imguiFramebufferInfo.width = m_swapchainExtent.width;
            imguiFramebufferInfo.height = m_swapchainExtent.height;
            imguiFramebufferInfo.layers = 1;
            VkFramebuffer framebuffer;

            if (vkCreateFramebuffer(device, &imguiFramebufferInfo, nullptr, &framebuffer) != VK_SUCCESS)
            {
                EE_CORE_ERROR("Failed to create ImGui framebuffer!");
            }
            else
            {
                m_imguiFramebuffers[i] = framebuffer;
                EE_CORE_INFO("ImGui framebuffer created");
            }
        }

        // Create the game framebuffers (offscreen rendering target)
        for (size_t i = 0; i < swapchainImageCount; ++i)
        {
            VkImageView gameColorAttachment = m_gameColorAttachmentImageViews[i];

            VkFramebufferCreateInfo gameFramebufferInfo{};
            gameFramebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
            gameFramebufferInfo.renderPass = renderPass;
            gameFramebufferInfo.attachmentCount = 1;
            gameFramebufferInfo.pAttachments = &gameColorAttachment;
            gameFramebufferInfo.width = m_swapchainExtent.width;
            gameFramebufferInfo.height = m_swapchainExtent.height;
            gameFramebufferInfo.layers = 1;

            VkFramebuffer framebuffer;

            if (vkCreateFramebuffer(device, &gameFramebufferInfo, nullptr, &framebuffer) != VK_SUCCESS)
            {
                EE_CORE_ERROR("Failed to create game framebuffer!");
            }
            else
            {
                m_gameFramebuffers[i] = framebuffer;
                EE_CORE_INFO("Game framebuffer created");
            }
        }
    }


    VkSurfaceFormatKHR VulkanSwapchain::ChooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats)
    {
        for (const auto& availableFormat : availableFormats)
        {
            if (availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB && availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
                return availableFormat;
            }
        }

        return availableFormats[0];
    }

    VkPresentModeKHR VulkanSwapchain::ChooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes)
    {
        for (const auto& availablePresentMode : availablePresentModes)
        {
            if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR)
            {
                return availablePresentMode;
            }
        }

        return VK_PRESENT_MODE_FIFO_KHR;
    }

    VkExtent2D VulkanSwapchain::ChooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities)
    {
        if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max())
        {
            return capabilities.currentExtent;
        }
        else
        {
            int width, height;
            glfwGetFramebufferSize(glfwGetCurrentContext(), &width, &height);

            VkExtent2D actualExtent =
            {
                static_cast<uint32_t>(width),
                static_cast<uint32_t>(height)
            };

            actualExtent.width = std::clamp(actualExtent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
            actualExtent.height = std::clamp(actualExtent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);

            return actualExtent;
        }
    }

    void VulkanSwapchain::Cleanup()
    {
        for (auto imageView : m_swapchainImageViews)
        {
            vkDestroyImageView(m_device, imageView, nullptr);
        }

        if (m_swapchain != VK_NULL_HANDLE)
        {
            vkDestroySwapchainKHR(m_device, m_swapchain, nullptr);
        }
    }

    void VulkanSwapchain::RecreateSwapchain()
    {
        vkDeviceWaitIdle(m_device); // Ensure no resources are in use before recreating

        Cleanup(); // Destroy old swapchain resources

        CreateSwapchain(); // Recreate the swapchain
    }

    uint32_t VulkanSwapchain::FindMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties)
    {

        VkPhysicalDeviceMemoryProperties memProperties;
        vkGetPhysicalDeviceMemoryProperties(m_physicalDevice, &memProperties);

        for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++)
        {
            if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties)
            {
                return i;
            }
        }
        EE_CORE_ASSERT(false, "Failed to find suitable memory type!");
    }

}
