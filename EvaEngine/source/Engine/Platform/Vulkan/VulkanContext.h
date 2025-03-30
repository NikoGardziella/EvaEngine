#pragma once

#include "Engine/Renderer/GraphicsContext.h"
#include <vulkan/vulkan.h>
#include <GLFW/glfw3.h>

namespace Engine {

    class VulkanContext : public GraphicsContext
    {
    public:
        VulkanContext(GLFWwindow* windowHandle);

        virtual void Init() override;
        virtual void SwapBuffers() override {}; // No SwapBuffers in Vulkan

    private:
        void CreateInstance();
        void CreateSurface();

        GLFWwindow* m_WindowHandle;
        VkInstance m_Instance;
        VkSurfaceKHR m_Surface;
    };
}


