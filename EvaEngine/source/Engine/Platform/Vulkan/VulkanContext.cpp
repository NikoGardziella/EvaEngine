#include "pch.h"
#include "VulkanContext.h"

#define VK_USE_PLATFORM_WIN32_KHR
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3native.h>

#include <vulkan/vulkan_win32.h>

#include "VulkanUtils.h"
#include <Engine/Renderer/Renderer.h>
#include "VulkanDescriptorPool.h"
#include <Engine/Core/Log.h>

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
        
		m_swapchain->Cleanup();
        vkDestroyRenderPass(m_deviceManager->GetDevice(), m_presentRenderPass, nullptr);

        if (m_vulkanInstance->GetInstance() != VK_NULL_HANDLE)
        {
            m_vulkanInstance->DestroyInstance();
        }
    }

    void VulkanContext::Shutdown()
    {
        delete s_instance;
        s_instance = nullptr;
    }

    void VulkanContext::Init()
    {
        CreateInstance();
        CreateSurface();
        SetupDevices();

        CreateSwapchain();
        CreateGraphicsQueue();
        CreatePresentRenderPass();
        CreateImGuiRenderPass();
        CreateGameRenderPass();


        CreateOffscreenRenderPass();

        CreateSwapchainFramebuffers();

        CreateCommandPool();
        CreateDescriptorPool();
        CreateImGuiDescriptorPool();

        CreateCommandBuffers();
        CreateSampler();
    
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

    void VulkanContext::CreateDescriptorPool() {
        EE_CORE_WARN("descriptor pool size not optimized, YET");

        VkDevice dev = m_deviceManager->GetDevice();

        // General graphics pool
        uint32_t maxSets = 200;
        uint32_t maxUniformBuffers = 100;
        uint32_t maxCombinedImageSamplers = 100;
        VkDescriptorPoolSize generalSizes[3];
        generalSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        generalSizes[0].descriptorCount = maxUniformBuffers;
        generalSizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        generalSizes[1].descriptorCount = maxCombinedImageSamplers;
        uint32_t lightBuffercount = 1;
        generalSizes[2].type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        generalSizes[2].descriptorCount = lightBuffercount;

        m_descriptorPool = std::make_shared<VulkanDescriptorPool>(dev, maxSets, generalSizes, 2, 0);
        m_lineDescriptorPool = std::make_shared<VulkanDescriptorPool>(dev, maxSets, generalSizes, 2, 0);

        // Compute/bindless pool
        uint32_t maxResidentLayers = 1024;
        uint32_t framesInFlight = MAX_FRAMES_IN_FLIGHT;

        // if you actually use update-after-bind, set the flag; otherwise 0
        VkDescriptorPoolCreateFlags computeFlags = 0; // or VK_DESCRIPTOR_POOL_CREATE_UPDATE_AFTER_BIND_BIT

        // adjust sizes to what your compute/bindless uses
        VkDescriptorPoolSize computeSizes[3];
        computeSizes[0].type = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
        computeSizes[0].descriptorCount = maxResidentLayers * framesInFlight * 2; // color + props

        computeSizes[1].type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        computeSizes[1].descriptorCount = framesInFlight * 3; // results + projectiles + mask

        computeSizes[2].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        computeSizes[2].descriptorCount = maxResidentLayers * framesInFlight; // if graphics samples color array

        m_computeDescPool = std::make_shared<VulkanDescriptorPool>(dev, framesInFlight, computeSizes, 3, computeFlags);
        m_effectDescPool = std::make_shared<VulkanDescriptorPool>(dev, framesInFlight, computeSizes, 3, computeFlags);
    

        VkDescriptorPoolCreateFlags falgs3d = 0;
        VkDescriptorPoolSize gfx3DSizes[5];

        // 0) Camera UBO  (binding 0: VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER)
        gfx3DSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        gfx3DSizes[0].descriptorCount = framesInFlight;              // 1 per frame

        // 1) Instance + Material SSBOs (bindings 1 and 3: VK_DESCRIPTOR_TYPE_STORAGE_BUFFER)
        gfx3DSizes[1].type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        gfx3DSizes[1].descriptorCount = framesInFlight * 2;          // instance + material per frame

        // 2) Albedo texture array (binding 2: VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER)
        gfx3DSizes[2].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        gfx3DSizes[2].descriptorCount = framesInFlight * MAX_ALBEDO_TEXTURES;

        uint32_t numberOfLightsbuffers = 1;
        gfx3DSizes[3].type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        gfx3DSizes[3].descriptorCount = numberOfLightsbuffers;

        gfx3DSizes[4].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        gfx3DSizes[4].descriptorCount = framesInFlight;

        m_descriptorPool3D = std::make_shared<VulkanDescriptorPool>(dev, framesInFlight, gfx3DSizes, 3, falgs3d);

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

    void VulkanContext::CreateOffscreenRenderPass()
    {
        VkAttachmentDescription colorAttachment{};
        colorAttachment.format = m_swapchain->GetSwapchainImageFormat();
        colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
        colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        colorAttachment.finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

        VkAttachmentReference colorAttachmentRef{};
        colorAttachmentRef.attachment = 0;
        colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

        VkSubpassDescription subpass{};
        subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
        subpass.colorAttachmentCount = 1;
        subpass.pColorAttachments = &colorAttachmentRef;

        VkSubpassDependency dependency{};
        dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
        dependency.dstSubpass = 0;
        dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        dependency.srcAccessMask = 0;
        dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

        VkRenderPassCreateInfo renderPassInfo{};
        renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
        renderPassInfo.attachmentCount = 1;
        renderPassInfo.pAttachments = &colorAttachment;
        renderPassInfo.subpassCount = 1;
        renderPassInfo.pSubpasses = &subpass;
        renderPassInfo.dependencyCount = 1;
        renderPassInfo.pDependencies = &dependency;

        if (vkCreateRenderPass(m_deviceManager->GetDevice(), &renderPassInfo, nullptr, &m_offscreenRenderPass) != VK_SUCCESS)
        {
            EE_CORE_ASSERT(false, "Failed to create offscreen render pass!");
        }
        else
        {
            EE_CORE_INFO("Offscreen render pass created");
        }
    }

    void VulkanContext::CreatePresentRenderPass()
    {
        // 1) Color attachment (swapchain)
        VkAttachmentDescription colorAttachment{};
        colorAttachment.format = m_swapchain->GetSwapchainImageFormat();
        colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
        colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

        VkAttachmentReference colorAttachmentRef{};
        colorAttachmentRef.attachment = 0; // index in attachments[]
        colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;





        // 3) Subpass: hook both color and depth
        VkSubpassDescription subpass{};
        subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
        subpass.colorAttachmentCount = 1;
        subpass.pColorAttachments = &colorAttachmentRef;

        // 4) Attachments array (color + depth)
        std::array<VkAttachmentDescription, 1> attachments = {
            colorAttachment,
        };

        VkRenderPassCreateInfo renderPassInfo{};
        renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
        renderPassInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
        renderPassInfo.pAttachments = attachments.data();
        renderPassInfo.subpassCount = 1;
        renderPassInfo.pSubpasses = &subpass;

        // 5) Subpass dependency (now also mentions depth)
        VkSubpassDependency dependency{};
        dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
        dependency.dstSubpass = 0;
        dependency.srcStageMask =
            VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT |
            VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
        dependency.srcAccessMask = 0;
        dependency.dstStageMask =
            VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT |
            VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
        dependency.dstAccessMask =
            VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT |
            VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

        renderPassInfo.dependencyCount = 1;
        renderPassInfo.pDependencies = &dependency;

        if (vkCreateRenderPass(m_deviceManager->GetDevice(), &renderPassInfo, nullptr, &m_presentRenderPass) != VK_SUCCESS)
        {
            EE_CORE_ASSERT(false, "Failed to create render pass!");
        }
        else
        {
            EE_CORE_INFO("Vulkan present render pass (color+depth) created");
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
        else
        {
            EE_CORE_INFO("Vulkan ImGui render pass created");
        }

    }

    void VulkanContext::CreateGameRenderPass()
    {
        VkAttachmentDescription colorAttachment{};
        colorAttachment.format = m_swapchain->GetSwapchainImageFormat(); // Must match the format of m_gameColorAttachmentImageViews
        colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
        colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        colorAttachment.finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

        VkAttachmentReference colorAttachmentRef{};
        colorAttachmentRef.attachment = 0;
        colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;


        VkFormat depthFormat = m_swapchain->GetDepthFormat();
        // 2) Depth attachment
        VkAttachmentDescription depthAttachment{};
        depthAttachment.format = depthFormat;
        depthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
        depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

        VkAttachmentReference depthAttachmentRef{};
        depthAttachmentRef.attachment = 1; // index in attachments[]
        depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;


        VkSubpassDescription subpass{};
        subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
        subpass.colorAttachmentCount = 1;
        subpass.pColorAttachments = &colorAttachmentRef;
        subpass.pDepthStencilAttachment = &depthAttachmentRef;

        VkSubpassDependency dependency{};
        dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
        dependency.dstSubpass = 0;
        dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        dependency.srcAccessMask = 0;
        dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

        std::array<VkAttachmentDescription, 2> attachments = {
              colorAttachment,
              depthAttachment
        };
        VkRenderPassCreateInfo renderPassInfo{};
        renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
        renderPassInfo.attachmentCount = attachments.size();
        renderPassInfo.pAttachments = attachments.data();
        renderPassInfo.subpassCount = 1;
        renderPassInfo.pSubpasses = &subpass;
        renderPassInfo.dependencyCount = 1;
        renderPassInfo.pDependencies = &dependency;

        if (vkCreateRenderPass(m_deviceManager->GetDevice(), &renderPassInfo, nullptr, &m_gameRenderPass) != VK_SUCCESS)
        {
            EE_CORE_ASSERT(false, "Failed to create Game Render Pass!");
        }
        else
        {
            EE_CORE_INFO("Vulkan Game render pass created");
        }
    }






    void VulkanContext::CreateSwapchainFramebuffers()
    {
        m_swapchain->CreateFramebuffers(m_presentRenderPass, m_imGuiRenderPass,m_gameRenderPass, m_deviceManager->GetDevice());
    }


    void VulkanContext::CreateSampler()
    {
        VkSamplerCreateInfo samplerInfo{};
        samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
        samplerInfo.magFilter = VK_FILTER_LINEAR;
        samplerInfo.minFilter = VK_FILTER_LINEAR;
        samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        samplerInfo.anisotropyEnable = VK_FALSE;
        samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
        samplerInfo.unnormalizedCoordinates = VK_FALSE;
        samplerInfo.compareEnable = VK_FALSE;
        samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;

        vkCreateSampler(m_deviceManager->GetDevice(), &samplerInfo, nullptr, &m_sampler);

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
