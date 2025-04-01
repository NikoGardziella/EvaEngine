#pragma once

#include <vulkan/vulkan.h>
#include <vector>

namespace Engine {

    class VulkanInstance {
    public:
        VulkanInstance(bool m_enableValidationLayers);
        ~VulkanInstance();

        void CreateInstance();
        void DestroyInstance();
        bool CheckValidationLayerSupport();
        std::vector<const char*> GetRequiredExtensions();

        void PopulateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo);

        static VKAPI_ATTR VkBool32 VKAPI_CALL DebugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageType, const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void* pUserData);

        void SetupDebugMessenger();

        VkResult CreateDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pDebugMessenger);

        void DestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks* pAllocator);

        VkInstance GetInstance() const { return m_instance; }

        const std::vector<const char*> GetValidationLayers() { return  m_validationLayers; };

		bool IsValidationLayersEnabled() { return m_enableValidationLayers; }

    private:
        VkInstance m_instance;
        VkDebugUtilsMessengerEXT m_debugMessenger;

        const std::vector<const char*> m_validationLayers =
        {
			// this same is in the VulkanContext.h
            "VK_LAYER_KHRONOS_validation"
        };


        bool m_enableValidationLayers = true;

    };

}
