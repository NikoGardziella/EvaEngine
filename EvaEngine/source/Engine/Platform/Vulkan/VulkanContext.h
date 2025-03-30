#pragma once

#include "Engine/Renderer/GraphicsContext.h"
#include <vulkan/vulkan.h>
#include <GLFW/glfw3.h>

namespace Engine {

    class VulkanContext : public GraphicsContext
    {
    public:
        VulkanContext(GLFWwindow* windowHandle);
        ~VulkanContext();

        virtual void Init() override;
        virtual void SwapBuffers() override {}; // No SwapBuffers in Vulkan
        static VulkanContext* Get();


        VkDevice& GetDevice() { return m_device;  }
        VkCommandPool& GetCommandPool() { return m_commandPool;  }
        VkQueue& GetGraphicsQueue() { return m_graphicsQueue;  }

        uint32_t FindMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties);

        VkCommandBuffer BeginSingleTimeCommands();
        void EndSingleTimeCommands(VkCommandBuffer commandBuffer);


        VkPhysicalDevice GetPhysicalDevice() { return m_vkPhysicalDevice; }
        VkCommandBuffer GetCommandBuffer();

    private:
        bool IsDeviceSuitable(VkPhysicalDevice device);
        void CreateInstance();
        void CreateSurface();
        void PickPhysicalDevice();
        void CreateLogicalDevice();
        void CreateCommandPool();
        void CreateGraphicsQueue();

    private:
        GLFWwindow* m_windowHandle;
        VkInstance m_instance;
        VkSurfaceKHR m_surface;
        VkDevice m_device;
        VkPhysicalDevice m_vkPhysicalDevice;
        VkCommandPool m_commandPool;
        VkQueue m_graphicsQueue;
        uint32_t m_GraphicsQueueFamilyIndex;

        static VulkanContext* s_instance;

    };

    
}


