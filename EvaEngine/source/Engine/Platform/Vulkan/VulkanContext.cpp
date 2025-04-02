#include "pch.h"
#include "VulkanContext.h"

#define VK_USE_PLATFORM_WIN32_KHR
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3native.h>

#include <vulkan/vulkan_win32.h>
#include <set>

#include "VulkanUtils.h"

namespace Engine {

    VulkanContext* VulkanContext::s_instance = nullptr;

    VulkanContext* VulkanContext::Get()
    {
        if (s_instance == nullptr)
        {
            EE_CORE_ASSERT(false, " no vulkan context instance");
        }
        return s_instance;
    }

    VulkanContext::VulkanContext(GLFWwindow* windowHandle)
        : m_windowHandle(windowHandle), m_surface(VK_NULL_HANDLE),
        m_commandPool(VK_NULL_HANDLE), m_graphicsQueue(VK_NULL_HANDLE)
    {
        s_instance = this;
        CreateInstance();
        CreateSurface();
        SetupDevices();
        
        CreateSwapchain();

        CreateCommandPool();
        CreateGraphicsQueue();
        CreateImageViews();
        CreateRenderPass();
    }

    VulkanContext::~VulkanContext()
    {
        // Cleanup Vulkan resources (destroy instance, surface, device, etc.)
        if (m_commandPool != VK_NULL_HANDLE)
        {
            vkDestroyCommandPool(m_deviceManager->GetDevice(), m_commandPool, nullptr);
        }
        if (m_deviceManager->GetDevice() != VK_NULL_HANDLE)
        {
            vkDestroyDevice(m_deviceManager->GetDevice(), nullptr);
        }
        if (m_surface != VK_NULL_HANDLE)
        {
            vkDestroySurfaceKHR(m_vulkanInstance->GetInstance(), m_surface, nullptr);
        }
        if (m_vulkanInstance->GetInstance() != VK_NULL_HANDLE)
        {
            m_vulkanInstance->DestroyInstance();
        }
		m_swapchain->Cleanup();
        vkDestroyRenderPass(m_deviceManager->GetDevice(), m_renderPass, nullptr);

    }

    void VulkanContext::Init()
    {
        // Vulkan initialization code here
    }

    void VulkanContext::CreateInstance()
    {
        m_vulkanInstance = new VulkanInstance(m_enableValidationLayers);
    }


    void VulkanContext::CreateSurface()
    {
        VkWin32SurfaceCreateInfoKHR createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
        createInfo.hwnd = glfwGetWin32Window(m_windowHandle);
        createInfo.hinstance = GetModuleHandle(nullptr);



        VkResult result = glfwCreateWindowSurface(m_vulkanInstance->GetInstance(), m_windowHandle, nullptr, &m_surface);
        if (result != VK_SUCCESS)
        {
            EE_CORE_INFO("Failed to create Vulkan surface! Error code:{} " + std::to_string(result));
        }
        else
        {
            EE_CORE_INFO("Vulkan window surface created");

        }

       
    }

    void VulkanContext::SetupDevices()
    {
		m_deviceManager = new VulkanDevice(m_vulkanInstance->GetInstance(), m_surface, m_enableValidationLayers);
    }

    

    void VulkanContext::CreateCommandPool()
    {
        VkCommandPoolCreateInfo poolCreateInfo = {};
        poolCreateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
       // poolCreateInfo.queueFamilyIndex = m_GraphicsQueueFamilyIndex;

        if (vkCreateCommandPool(m_deviceManager->GetDevice(), &poolCreateInfo, nullptr, &m_commandPool) != VK_SUCCESS)
        {
            throw std::runtime_error("Failed to create Vulkan command pool!");
        }
        EE_CORE_INFO("Vulkan command pool create");

    }

    void VulkanContext::CreateGraphicsQueue()
    {
        vkGetDeviceQueue(m_deviceManager->GetDevice(), 0, 0, &m_graphicsQueue);  // Get the first queue from the device
        EE_CORE_INFO("Vulkan graphics queue created");
    }

    void VulkanContext::CreateSwapchain()
    {
		m_swapchain = new VulkanSwapchain(m_deviceManager->GetDevice(), m_surface, m_deviceManager->GetPhysicalDevice());
    }

    void VulkanContext::CreateRenderPass()
    {
        VkAttachmentDescription colorAttachment{};
        colorAttachment.format = m_swapchain->GetSwapchainImageFormat();  // Swapchain format
        colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
        colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

        VkAttachmentReference colorAttachmentRef{};
        colorAttachmentRef.attachment = 0;
        colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

        VkSubpassDescription subpass{};
        subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
        subpass.colorAttachmentCount = 1;
        subpass.pColorAttachments = &colorAttachmentRef;

        VkRenderPassCreateInfo renderPassInfo{};
        renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
        renderPassInfo.attachmentCount = 1;
        renderPassInfo.pAttachments = &colorAttachment;
        renderPassInfo.subpassCount = 1;
        renderPassInfo.pSubpasses = &subpass;

        if (vkCreateRenderPass(m_deviceManager->GetDevice(), &renderPassInfo, nullptr, &m_renderPass) != VK_SUCCESS)
        {
			EE_CORE_ASSERT(false, "Failed to create render pass!");
        }
        else
        {
			EE_CORE_INFO("Vulkan render pass created");
        }
    }


    void VulkanContext::CreateImageViews()
    {
        //m_swapchainImageViews.resize(m_swapchainImages.size());
        m_swapchainImageViews.resize(m_swapchain->GetSwapchainImages().size());

        for (size_t i = 0; i < m_swapchain->GetSwapchainImages().size(); ++i) {
            VkImageViewCreateInfo viewCreateInfo = {};
            viewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
            viewCreateInfo.image = m_swapchain->GetSwapchainImages()[i];
            viewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
            viewCreateInfo.format = m_swapchain->GetSwapchainImageFormat();
            viewCreateInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
            viewCreateInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
            viewCreateInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
            viewCreateInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
            viewCreateInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            viewCreateInfo.subresourceRange.baseMipLevel = 0;
            viewCreateInfo.subresourceRange.levelCount = 1;
            viewCreateInfo.subresourceRange.baseArrayLayer = 0;
            viewCreateInfo.subresourceRange.layerCount = 1;

            if (vkCreateImageView(m_deviceManager->GetDevice(), &viewCreateInfo, nullptr, &m_swapchainImageViews[i]) != VK_SUCCESS)
            {
                EE_CORE_ERROR("Failed to create image views for swapchain images");
            }
            EE_CORE_INFO("Vulkan image views for swapchain image created");

        }
    }


    uint32_t VulkanContext::FindMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties)
    {

        VkPhysicalDeviceMemoryProperties memProperties;
        vkGetPhysicalDeviceMemoryProperties(m_deviceManager->GetPhysicalDevice(), &memProperties);

        for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++)
        {
            if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties)
            {
                EE_CORE_INFO("Vulkan memory type found at index: {}", i);
                return i;
            }
        }

        throw std::runtime_error("Failed to find a suitable memory type!");
    }

    VkCommandBuffer VulkanContext::BeginSingleTimeCommands()
    {
        VkCommandBufferAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        allocInfo.commandPool = m_commandPool;
        allocInfo.commandBufferCount = 1;

        VkCommandBuffer commandBuffer;
        vkAllocateCommandBuffers(m_deviceManager->GetDevice(), &allocInfo, &commandBuffer);

        VkCommandBufferBeginInfo beginInfo{};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

        vkBeginCommandBuffer(commandBuffer, &beginInfo);
        return commandBuffer;
    }


    void VulkanContext::EndSingleTimeCommands(VkCommandBuffer commandBuffer)
    {
        vkEndCommandBuffer(commandBuffer);

        VkSubmitInfo submitInfo{};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &commandBuffer;

        VkQueue graphicsQueue; // You need to retrieve your graphics queue
       // vkGetDeviceQueue(m_device, m_GraphicsQueueFamilyIndex, 0, &graphicsQueue);

        vkQueueSubmit(graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
        vkQueueWaitIdle(graphicsQueue);

        vkFreeCommandBuffers(m_deviceManager->GetDevice(), m_commandPool, 1, &commandBuffer);
    }


    VkCommandBuffer VulkanContext::GetCommandBuffer()
    {
        VkCommandBufferAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        allocInfo.commandPool = m_commandPool;
        allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        allocInfo.commandBufferCount = 1;

        VkCommandBuffer commandBuffer;
        if (vkAllocateCommandBuffers(m_deviceManager->GetDevice(), &allocInfo, &commandBuffer) != VK_SUCCESS)
        {
            throw std::runtime_error("Failed to allocate command buffer!");
        }

        return commandBuffer;
    }


 



  

    


   







}
