#pragma once

#include "Engine/Renderer/GraphicsContext.h"
#include <vulkan/vulkan.h>
#include <GLFW/glfw3.h>
#include <optional>

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


        VkDevice& GetDevice() { return m_device;  }
        VkCommandPool& GetCommandPool() { return m_commandPool;  }
        VkQueue& GetGraphicsQueue() { return m_graphicsQueue;  }

        uint32_t FindMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties);

        VkCommandBuffer BeginSingleTimeCommands();
        void EndSingleTimeCommands(VkCommandBuffer commandBuffer);


        VkPhysicalDevice GetPhysicalDevice() { return m_physicalDevice; }
        VkCommandBuffer GetCommandBuffer();

    private:
        bool IsDeviceSuitable(VkPhysicalDevice device);
        void CheckSwapchainSupport();
        void CreateInstance();
        void CreateSurface();
        void PickPhysicalDevice();
        void CreateLogicalDevice();
        void CreateCommandPool();
        void CreateGraphicsQueue();

        void CreateSwapchain();

        void CreateImageViews();

        VulkanContext::SwapChainSupportDetails QuerySwapChainSupport(VkPhysicalDevice device);
        VkSurfaceFormatKHR ChooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats);
        VkPresentModeKHR ChooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes);
        VkExtent2D ChooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities);
        QueueFamilyIndices FindQueueFamilies(VkPhysicalDevice device);
        bool CheckValidationLayerSupport();
    private:
        GLFWwindow* m_windowHandle;
        VkInstance m_instance;
        VkSurfaceKHR m_surface;
        VkDevice m_device;
        VkPhysicalDevice m_physicalDevice;
        VkCommandPool m_commandPool;
        VkQueue m_graphicsQueue;

        VkSwapchainKHR m_swapchain;
        std::vector<VkImage> m_swapchainImages;
        std::vector<VkImageView> m_swapchainImageViews;
        VkFormat m_swapchainImageFormat;
        VkExtent2D m_swapchainExtent;
        VkQueue m_presentQueue;

        static VulkanContext* s_instance;
        SwapChainSupportDetails m_swapChainSupportDetails;

        VkPhysicalDeviceFeatures deviceFeatures{};


        const std::vector<const char*> m_validationLayers =
        {
            "VK_LAYER_KHRONOS_validation"
        };

    #ifdef NDEBUG
            const bool enableValidationLayers = false;
    #else
            const bool enableValidationLayers = true;
    #endif
    };

    
}


