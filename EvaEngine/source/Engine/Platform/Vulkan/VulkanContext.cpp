#include "pch.h"
#include "VulkanContext.h"

#include <vector>
#include <iostream>

namespace Engine {

    VulkanContext::VulkanContext(GLFWwindow* windowHandle)
        : m_WindowHandle(windowHandle), m_Instance(VK_NULL_HANDLE), m_Surface(VK_NULL_HANDLE)
    {
        if (!windowHandle)
        {
            EE_CORE_ERROR("Failed to initialize VulkanContext: windowHandle is NULL");
        }
    }

    void VulkanContext::Init()
    {
        EE_PROFILE_FUNCTION();
        EE_CORE_INFO("Initializing Vulkan Context");

        CreateInstance();
        CreateSurface();

        EE_CORE_INFO("VulkanContext initialized successfully.");
    }

    void VulkanContext::CreateInstance()
    {
        EE_PROFILE_FUNCTION();

        VkApplicationInfo appInfo{};
        appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
        appInfo.pApplicationName = "Vulkan Application";
        appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
        appInfo.pEngineName = "Custom Engine";
        appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
        appInfo.apiVersion = VK_API_VERSION_1_2;

        // Get required extensions from GLFW
        uint32_t glfwExtensionCount = 0;
        const char** glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

        VkInstanceCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
        createInfo.pApplicationInfo = &appInfo;
        createInfo.enabledExtensionCount = glfwExtensionCount;
        createInfo.ppEnabledExtensionNames = glfwExtensions;

        VkResult result = vkCreateInstance(&createInfo, nullptr, &m_Instance);
        if (result != VK_SUCCESS) {
            EE_CORE_ERROR("Failed to create Vulkan instance! Error Code");
        }
        else {
            EE_CORE_INFO("Vulkan instance created successfully.");
        }
    }

    void VulkanContext::CreateSurface()
    {
        EE_PROFILE_FUNCTION();

        if (glfwCreateWindowSurface(m_Instance, m_WindowHandle, nullptr, &m_Surface) != VK_SUCCESS) {
            EE_CORE_ERROR("Failed to create Vulkan surface!");
        }
        else {
            EE_CORE_INFO("Vulkan surface created successfully.");
        }
    }
}
