#pragma once

#include "Engine/Renderer/GraphicsContext.h"
#include <vulkan/vulkan.h>
#include <GLFW/glfw3.h>
#include <optional>
#include "VulkanInstance.h"
#include "VulkanDevice.h"
#include "VulkanSwapchain.h"
#include "VulkanDescriptorPool.h"

namespace Engine {

    class VulkanContext : public GraphicsContext
    {
    public:


        VulkanContext(GLFWwindow* windowHandle);
        ~VulkanContext();

        struct SwapChainSupportDetails
        {
            VkSurfaceCapabilitiesKHR capabilities;
            std::vector<VkSurfaceFormatKHR> formats;
            std::vector<VkPresentModeKHR> presentModes;
        };

        struct QueueFamilyIndices
        {
            std::optional<uint32_t> graphicsFamily;
            std::optional<uint32_t> presentFamily;

            bool isComplete()
            {
                return graphicsFamily.has_value() && presentFamily.has_value();
            }
        };

       

        virtual void Init() override;
        virtual void SwapBuffers() override {}; // No SwapBuffers in Vulkan
        static VulkanContext* Get();


        VkCommandPool& GetCommandPool() { return m_commandPool;  }
        VkQueue& GetGraphicsQueue() { return m_graphicsQueue;  }

        uint32_t FindMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties);

        VkCommandBuffer BeginSingleTimeCommands();
        void EndSingleTimeCommands(VkCommandBuffer commandBuffer);

        void CreateCommandBuffers();


        //VkPhysicalDevice GetPhysicalDevice() { return m_physicalDevice; }
        VkFormat FindDepthFormat();
        void CreateDepthAttachment();
        void CreateImGuiDescriptorPool();

        VulkanDevice& GetDeviceManager() const { return *m_deviceManager; }
        VkRenderPass& GetRenderPass() { return m_renderPass; }
		VkRenderPass& GetImGuiRenderPass() { return m_imGuiRenderPass; }
        VulkanSwapchain& GetVulkanSwapchain() { return *m_swapchain; }
        //std::vector<VkFramebuffer>& GetSwapchainFramebuffers() { return m_swapchainFramebuffers; }
		VulkanInstance& GetVulkanInstance() { return *m_vulkanInstance; }
        VkFramebuffer& GetSwapchainFramebuffer(uint32_t imageIndex ) { return m_swapchainFramebuffers[imageIndex]; }
		VkFramebuffer& GetImGuiFramebuffer(uint32_t imageIndex) { return m_imguiFramebuffers[imageIndex]; }

       // VkCommandBuffer VulkanContext::GetCurrentCommandBuffer() { return m_commandBuffers[Renderer::GetCurrentFrame()];   }
        VkCommandBuffer& GetCommandBuffer(uint32_t imageIndex) { return m_commandBuffers[imageIndex]; }
        VkDescriptorPool GetDescriptorPool() { return m_descriptorPool->GetDescriptorPool(); }
		VkDescriptorPool& GetImGuiDescriptorPool() { return m_imguiDescriptorPool; }


    private:
        void CreateInstance();
        void CreateSurface();
        void SetupDevices();
        void CreateGraphicsQueue();
        void CreateSwapchain();

        void CreateRenderPass();
        void CreateImGuiRenderPass();

        void CreateImageViews();
        void CreateFramebuffers();
        void CreateCommandPool();
        void CreateDescriptorPool();

        void CreateEntityIDAttachment();



    private:

    private:
        GLFWwindow* m_windowHandle;
        VulkanInstance* m_vulkanInstance;
        VkSurfaceKHR m_surface;
        VulkanDevice* m_deviceManager;
		Ref<VulkanDescriptorPool> m_descriptorPool;
        VkDescriptorPool m_imguiDescriptorPool;

        VulkanSwapchain* m_swapchain;
        std::vector<VkImageView> m_swapchainImageViews;
        
        std::vector<VkCommandBuffer> m_commandBuffers;
        SwapChainSupportDetails m_swapChainSupportDetails;
        VkCommandPool m_commandPool;
        VkQueue m_graphicsQueue;
        VkRenderPass m_renderPass;
        VkRenderPass m_imGuiRenderPass;

        std::vector<VkFramebuffer> m_swapchainFramebuffers;
        std::vector<VkFramebuffer> m_imguiFramebuffers;
      

        static VulkanContext* s_instance;

        VkImage m_entityIDImage;
        VkImage m_depthImage;
        VkImageView m_entityIDImageView;
        VkImageView m_depthAttachmentView;
        VkDeviceMemory m_depthImageMemory;
        VkDeviceMemory m_entityIDImageMemory;


        //VkDebugUtilsMessengerEXT m_debugMessenger;
        const std::vector<const char*> m_validationLayers =
        {
            "VK_LAYER_KHRONOS_validation"
        };
        const std::vector<const char*> m_deviceExtensions =
        {
            VK_KHR_SWAPCHAIN_EXTENSION_NAME
        };

    #ifdef NDEBUG
            const bool m_enableValidationLayers = false;
    #else
            const bool m_enableValidationLayers = true;
    #endif
    };

    
}


