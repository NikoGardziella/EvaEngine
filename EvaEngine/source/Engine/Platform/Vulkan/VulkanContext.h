#pragma once

#include "Engine/Renderer/GraphicsContext.h"
#include <vulkan/vulkan.h>
#include <GLFW/glfw3.h>
#include <optional>
#include "VulkanInstance.h"
#include "VulkanDevice.h"

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

    private:
        void CheckSwapchainSupport();
        void CreateInstance();
        void CreateSurface();
        void SetupDevices();
        void CreateCommandPool();
        void CreateGraphicsQueue();

        void CreateSwapchain();

        void CreateImageViews();
    private:

        VkSurfaceFormatKHR ChooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats);
        VkPresentModeKHR ChooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes);
        VkExtent2D ChooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities);
        bool CheckValidationLayerSupport();
        std::vector<const char*> GetRequiredExtensions();
        void PopulateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo);
    private:
        GLFWwindow* m_windowHandle;

        VulkanInstance* m_vulkanInstance;

        VkSurfaceKHR m_surface;
        //VkPhysicalDevice m_physicalDevice;

        VulkanDevice* m_deviceManager;
        VkCommandPool m_commandPool;
        VkQueue m_graphicsQueue;

        VkSwapchainKHR m_swapChain;
        std::vector<VkImage> m_swapchainImages;
        std::vector<VkImageView> m_swapchainImageViews;
        VkFormat m_swapchainImageFormat;
        VkExtent2D m_swapchainExtent;
        VkQueue m_presentQueue;

        static VulkanContext* s_instance;


        SwapChainSupportDetails m_swapChainSupportDetails;


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


