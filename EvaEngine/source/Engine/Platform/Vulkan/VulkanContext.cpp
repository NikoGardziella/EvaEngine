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
#include <Engine/Renderer/Renderer.h>

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
        CreateInstance();
        CreateSurface();
        SetupDevices();

        CreateSwapchain();

        CreateGraphicsQueue();
        CreateImageViews();
        CreateRenderPass();
        CreateImGuiRenderPass();

        CreateFramebuffers();
        CreateCommandPool();
        CreateDescriptorPool();
        CreateImGuiDescriptorPool();

        CreateCommandBuffers();
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
        // manage the memory that is used to store the buffers and command buffers are allocated from them.
        QueueFamilyIndices queueFamilyIndices = VulkanUtils::FindQueueFamilies(m_deviceManager->GetPhysicalDevice(), m_surface);
        VkCommandPoolCreateInfo poolInfo{};
        poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
        poolInfo.queueFamilyIndex = queueFamilyIndices.graphicsFamily.value();

        if (vkCreateCommandPool(m_deviceManager->GetDevice(), &poolInfo, nullptr, &m_commandPool) != VK_SUCCESS)
        {
			EE_CORE_ASSERT(false, "Failed to create command pool!");    
        }
        else
        {
            EE_CORE_INFO("Vulkan command pool create");
        }

    }

    void VulkanContext::CreateDescriptorPool()
    {
        // Adjust the numbers as needed to ensure the pool is large enough for both normal rendering and ImGui
        uint32_t maxSets = 200; // Total number of descriptor sets
        uint32_t maxUniformBuffers = 100; // Number of uniform buffers
        uint32_t maxCombinedImageSamplers = 100; // Number of combined image samplers

        m_descriptorPool = std::make_shared<VulkanDescriptorPool>(m_deviceManager->GetDevice(), maxSets, maxUniformBuffers, maxCombinedImageSamplers);
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
        // First color attachment (o_Color)
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
        colorAttachmentRef.attachment = 0;  // Correct index
        colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;


        // Subpass description
        VkSubpassDescription subpass{};
        subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
        subpass.colorAttachmentCount = 1;
        VkAttachmentReference colorAttachments[] = { colorAttachmentRef };
        subpass.pColorAttachments = colorAttachments;

        // Render pass creation
        std::array<VkAttachmentDescription, 1> attachments = { colorAttachment };
        VkRenderPassCreateInfo renderPassInfo{};
        renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
        renderPassInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
        renderPassInfo.pAttachments = attachments.data();
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

    void VulkanContext::CreateImGuiRenderPass()
    {
        VkAttachmentDescription colorAttachment = {};
        colorAttachment.format = m_swapchain->GetSwapchainImageFormat();
        colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
        colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;  
        colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        colorAttachment.initialLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
        colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

        VkAttachmentReference colorAttachmentRef = {};
        colorAttachmentRef.attachment = 0;
        colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

        VkSubpassDescription subpass = {};
        subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
        subpass.colorAttachmentCount = 1;
        subpass.pColorAttachments = &colorAttachmentRef;

        VkSubpassDependency dependency = {};
        dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
        dependency.dstSubpass = 0;
        dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        dependency.srcAccessMask = 0;
        dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

        VkRenderPassCreateInfo renderPassInfo = {};
        renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
        renderPassInfo.attachmentCount = 1;
        renderPassInfo.pAttachments = &colorAttachment;
        renderPassInfo.subpassCount = 1;
        renderPassInfo.pSubpasses = &subpass;
        renderPassInfo.dependencyCount = 1;
        renderPassInfo.pDependencies = &dependency;

        if (vkCreateRenderPass(m_deviceManager->GetDevice(), &renderPassInfo, nullptr, &m_imGuiRenderPass) != VK_SUCCESS)
        {
			EE_CORE_ASSERT(false, "Failed to create ImGui render pass!");
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

    void VulkanContext::CreateFramebuffers()
    {
        // Resize framebuffer vectors based on the number of swapchain images
        size_t swapchainImageCount = m_swapchain->GetSwapchainImageViews().size();
        m_swapchainFramebuffers.resize(swapchainImageCount);
        m_imguiFramebuffers.resize(swapchainImageCount);

        // Create the scene framebuffers
        for (size_t i = 0; i < swapchainImageCount; ++i)
        {
            VkImageView swapchainImageView = m_swapchain->GetSwapchainImageViews()[i];

            // === Scene framebuffer ===
            VkImageView sceneAttachments[] = { swapchainImageView };

            VkFramebufferCreateInfo sceneFramebufferInfo = {};
            sceneFramebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
            sceneFramebufferInfo.renderPass = m_renderPass; // Main scene render pass
            sceneFramebufferInfo.attachmentCount = static_cast<uint32_t>(std::size(sceneAttachments));
            sceneFramebufferInfo.pAttachments = sceneAttachments;
            sceneFramebufferInfo.width = m_swapchain->GetSwapchainExtent().width;
            sceneFramebufferInfo.height = m_swapchain->GetSwapchainExtent().height;
            sceneFramebufferInfo.layers = 1;

            if (vkCreateFramebuffer(m_deviceManager->GetDevice(), &sceneFramebufferInfo, nullptr, &m_swapchainFramebuffers[i]) != VK_SUCCESS)
            {
                EE_CORE_ERROR("Failed to create scene framebuffer!");
            }
            else
            {
                EE_CORE_INFO("Scene framebuffer created");
            }
        }

        // Create the ImGui framebuffers
        for (size_t i = 0; i < swapchainImageCount; ++i)
        {
            VkImageView imguiImageView = m_swapchain->GetSwapchainImageViews()[i]; // Same swapchain image

            // === ImGui framebuffer ===
            VkImageView imguiAttachments[] = { imguiImageView };

            VkFramebufferCreateInfo imguiFramebufferInfo = {};
            imguiFramebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
            imguiFramebufferInfo.renderPass = m_imGuiRenderPass; // ImGui render pass
            imguiFramebufferInfo.attachmentCount = static_cast<uint32_t>(std::size(imguiAttachments));
            imguiFramebufferInfo.pAttachments = imguiAttachments;
            imguiFramebufferInfo.width = m_swapchain->GetSwapchainExtent().width;
            imguiFramebufferInfo.height = m_swapchain->GetSwapchainExtent().height;
            imguiFramebufferInfo.layers = 1;

            if (vkCreateFramebuffer(m_deviceManager->GetDevice(), &imguiFramebufferInfo, nullptr, &m_imguiFramebuffers[i]) != VK_SUCCESS)
            {
                EE_CORE_ERROR("Failed to create ImGui framebuffer!");
            }
            else
            {
                EE_CORE_INFO("ImGui framebuffer created");
            }
        }
    }





    void VulkanContext::CreateEntityIDAttachment()
    {
        VkImageCreateInfo imageInfo{};
        imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        imageInfo.imageType = VK_IMAGE_TYPE_2D;
        imageInfo.format = VK_FORMAT_R32_SINT;  // Store entity IDs as unsigned integers
        imageInfo.extent.width = m_swapchain->GetSwapchainExtent().width;
        imageInfo.extent.height = m_swapchain->GetSwapchainExtent().height;
        imageInfo.extent.depth = 1;
        imageInfo.mipLevels = 1;
        imageInfo.arrayLayers = 1;
        imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
        imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
        imageInfo.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
        imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

        if (vkCreateImage(m_deviceManager->GetDevice(), &imageInfo, nullptr, &m_entityIDImage) != VK_SUCCESS)
        {
            EE_CORE_ERROR("Failed to create Entity ID image!");
        }

        // Allocate memory for image
        VkMemoryRequirements memRequirements;
        vkGetImageMemoryRequirements(m_deviceManager->GetDevice(), m_entityIDImage, &memRequirements);

        VkMemoryAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        allocInfo.allocationSize = memRequirements.size;
        allocInfo.memoryTypeIndex = FindMemoryType(memRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

        if (vkAllocateMemory(m_deviceManager->GetDevice(), &allocInfo, nullptr, &m_entityIDImageMemory) != VK_SUCCESS)
        {
            EE_CORE_ERROR("Failed to allocate memory for Entity ID image!");
        }

        // Bind image to allocated memory
        vkBindImageMemory(m_deviceManager->GetDevice(), m_entityIDImage, m_entityIDImageMemory, 0);

        // Create Image View
        VkImageViewCreateInfo viewInfo{};
        viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        viewInfo.image = m_entityIDImage;
        viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        viewInfo.format = VK_FORMAT_R32_SINT;
        viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        viewInfo.subresourceRange.baseMipLevel = 0;
        viewInfo.subresourceRange.levelCount = 1;
        viewInfo.subresourceRange.baseArrayLayer = 0;
        viewInfo.subresourceRange.layerCount = 1;

        if (vkCreateImageView(m_deviceManager->GetDevice(), &viewInfo, nullptr, &m_entityIDImageView) != VK_SUCCESS)
        {
            EE_CORE_ERROR("Failed to create Entity ID image view!");
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
                return i;
            }
        }
		EE_CORE_ASSERT(false, "Failed to find suitable memory type!");
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


        vkQueueSubmit(m_graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
        vkQueueWaitIdle(m_graphicsQueue);

        vkFreeCommandBuffers(m_deviceManager->GetDevice(), m_commandPool, 1, &commandBuffer);
    }

    void VulkanContext::CreateCommandBuffers()
    {
        m_commandBuffers.resize(MAX_FRAMES_IN_FLIGHT);

        VkCommandBufferAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        allocInfo.commandPool = m_commandPool;
        allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        allocInfo.commandBufferCount = static_cast<uint32_t>(m_commandBuffers.size());

        if (vkAllocateCommandBuffers(m_deviceManager->GetDevice(), &allocInfo, m_commandBuffers.data()) != VK_SUCCESS)
        {
			EE_CORE_ASSERT(false, "Failed to allocate command buffers!");
        }
    }

   

    VkFormat VulkanContext::FindDepthFormat()
    {
        std::vector<VkFormat> candidates = {
            VK_FORMAT_D32_SFLOAT,
            VK_FORMAT_D32_SFLOAT_S8_UINT,
            VK_FORMAT_D24_UNORM_S8_UINT
        };

        for (VkFormat format : candidates)
        {
            VkFormatProperties props;
            vkGetPhysicalDeviceFormatProperties(m_deviceManager->GetPhysicalDevice(), format, &props);

            if (props.optimalTilingFeatures & VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT)
            {
                return format;
            }
        }

        EE_CORE_ERROR("Failed to find suitable depth format!");
        return VK_FORMAT_UNDEFINED;
    }

    void VulkanContext::CreateDepthAttachment()
    {
        VkFormat depthFormat = FindDepthFormat();

        VkImageCreateInfo imageInfo{};
        imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        imageInfo.imageType = VK_IMAGE_TYPE_2D;
        imageInfo.format = depthFormat;
        imageInfo.extent.width = m_swapchain->GetSwapchainExtent().width;
        imageInfo.extent.height = m_swapchain->GetSwapchainExtent().height;
        imageInfo.extent.depth = 1;
        imageInfo.mipLevels = 1;
        imageInfo.arrayLayers = 1;
        imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
        imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
        imageInfo.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
        imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

        if (vkCreateImage(m_deviceManager->GetDevice(), &imageInfo, nullptr, &m_depthImage) != VK_SUCCESS)
        {
            EE_CORE_ERROR("Failed to create depth image!");
        
        }

        vkBindImageMemory(m_deviceManager->GetDevice(), m_depthImage, m_depthImageMemory, 0);

        // Create Depth Image View
        VkImageViewCreateInfo viewInfo{};
        viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        viewInfo.image = m_depthImage;
        viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        viewInfo.format = depthFormat;
        viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
        viewInfo.subresourceRange.baseMipLevel = 0;
        viewInfo.subresourceRange.levelCount = 1;
        viewInfo.subresourceRange.baseArrayLayer = 0;
        viewInfo.subresourceRange.layerCount = 1;

        if (vkCreateImageView(m_deviceManager->GetDevice(), &viewInfo, nullptr, &m_depthAttachmentView) != VK_SUCCESS)
        {
            EE_CORE_ERROR("Failed to create depth image view!");
        }
    }

    void VulkanContext::CreateImGuiDescriptorPool()
    {
        std::array<VkDescriptorPoolSize, 11> poolSizes = {
            VkDescriptorPoolSize{ VK_DESCRIPTOR_TYPE_SAMPLER, 1000 },
            VkDescriptorPoolSize{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000 },
            VkDescriptorPoolSize{ VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1000 },
            VkDescriptorPoolSize{ VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1000 },
            VkDescriptorPoolSize{ VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 1000 },
            VkDescriptorPoolSize{ VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 1000 },
            VkDescriptorPoolSize{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1000 },
            VkDescriptorPoolSize{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1000 },
            VkDescriptorPoolSize{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1000 },
            VkDescriptorPoolSize{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1000 },
            VkDescriptorPoolSize{ VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1000 },
        };

        VkDescriptorPoolCreateInfo poolInfo{};
        poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        poolInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
        poolInfo.maxSets = 1000 * static_cast<uint32_t>(poolSizes.size());
        poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
        poolInfo.pPoolSizes = poolSizes.data();

        if (vkCreateDescriptorPool(m_deviceManager->GetDevice(), &poolInfo, nullptr, &m_imguiDescriptorPool) != VK_SUCCESS)
        {
		    EE_CORE_ASSERT(false, "Failed to create ImGui descriptor pool!");
        }
    }

 




}
