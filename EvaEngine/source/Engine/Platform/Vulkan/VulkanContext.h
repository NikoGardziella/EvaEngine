#pragma once

#include "Engine/Renderer/GraphicsContext.h"
#include <vulkan/vulkan.h>
#include <GLFW/glfw3.h>
#include <optional>
#include "VulkanInstance.h"
#include "VulkanDevice.h"
#include "VulkanSwapchain.h"

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


        //VkPhysicalDevice GetPhysicalDevice() { return m_physicalDevice; }
        VkCommandBuffer GetCommandBuffer();
        VulkanDevice& GetDeviceManager() const { return *m_deviceManager; }
        VkRenderPass& GetRenderPass() { return m_renderPass; }
        VulkanSwapchain& GetSwapchain() { return *m_swapchain; }

    private:
        void CreateInstance();
        void CreateSurface();
        void SetupDevices();
        void CreateCommandPool();
        void CreateGraphicsQueue();
        void CreateSwapchain();
        void CreateRenderPass();

        void CreateImageViews();
    private:

    private:
        GLFWwindow* m_windowHandle;
        VulkanInstance* m_vulkanInstance;
        VkSurfaceKHR m_surface;
        VulkanDevice* m_deviceManager;


        VulkanSwapchain* m_swapchain;
        std::vector<VkImageView> m_swapchainImageViews;
        

        SwapChainSupportDetails m_swapChainSupportDetails;
        VkCommandPool m_commandPool;
        VkQueue m_graphicsQueue;
        VkRenderPass m_renderPass;

      
        VkQueue m_presentQueue;

        static VulkanContext* s_instance;




        VkDebugUtilsMessengerEXT m_debugMessenger;
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


