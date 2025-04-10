
#pragma once


#include <vulkan/vulkan.h>
#include <vector>
#include <string>

namespace Engine {

    class VulkanDevice {
    public:
        VulkanDevice(VkInstance instance, VkSurfaceKHR surface, bool m_enableValidationLayers);
        ~VulkanDevice() = default;

        VkPhysicalDevice GetPhysicalDevice() const { return m_physicalDevice; }
        VkDevice GetDevice() const { return m_device; }
        
        VkQueue GetGraphicsQueue() const { return m_graphicsQueue; }

		uint32_t GetGraphicsQueueFamilyIndex() const { return m_graphicsQueueFamilyIndex; }

    private:
        void PickPhysicalDevice();
        void CreateLogicalDevice();

        bool IsDeviceSuitable(VkPhysicalDevice device);
        bool CheckDeviceExtensionSupport(VkPhysicalDevice device);



        VkPhysicalDevice m_physicalDevice = VK_NULL_HANDLE;
        VkDevice m_device = VK_NULL_HANDLE;
        VkQueue m_graphicsQueue = VK_NULL_HANDLE;
        VkCommandPool m_commandPool = VK_NULL_HANDLE;

        VkInstance m_instance;
        VkSurfaceKHR m_surface;
        VkPhysicalDeviceFeatures m_deviceFeatures{};
        VkQueue m_presentQueue;

		uint32_t m_graphicsQueueFamilyIndex = 0;

        bool m_enableValidationLayers = true;

        const std::vector<const char*> m_deviceExtensions =
        {
            VK_KHR_SWAPCHAIN_EXTENSION_NAME
        };

        const std::vector<const char*> m_validationLayers =
        {
            "VK_LAYER_KHRONOS_validation"
        };
    };
}
