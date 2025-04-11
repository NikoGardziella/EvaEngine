#pragma once

#pragma once

#include <vulkan/vulkan.h>
#include <vector>

namespace Engine {

    class VulkanSwapchain
    {
    public:
        VulkanSwapchain(VkDevice device, VkSurfaceKHR surface, VkPhysicalDevice physicalDevice);
        ~VulkanSwapchain();

        void CreateSwapchain();
        void Cleanup();

        void RecreateSwapchain();

        VkSwapchainKHR GetSwapchain() const { return m_swapchain; }
        VkFormat GetSwapchainImageFormat() const { return m_swapchainImageFormat; }
        VkExtent2D GetSwapchainExtent() const { return m_swapchainExtent; }
        const std::vector<VkImageView>& GetSwapchainImageViews() const { return m_swapchainImageViews; }
		const std::vector<VkImageView>& GetImGuiImageViews() const { return m_imguiImageViews; }
        const std::vector<VkImage>& GetSwapchainImages() const { return m_swapchainImages; }

    private:
        void CreateImageViews();
        VkSurfaceFormatKHR ChooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats);
        VkPresentModeKHR ChooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes);
        VkExtent2D ChooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities);

    private:
        VkDevice m_device;
        VkSurfaceKHR m_surface;
        VkPhysicalDevice m_physicalDevice;

        VkSwapchainKHR m_swapchain;
        std::vector<VkImage> m_swapchainImages;

        std::vector<VkImageView> m_swapchainImageViews;
        std::vector<VkImageView> m_imguiImageViews;

        VkFormat m_swapchainImageFormat;
        VkExtent2D m_swapchainExtent;
    };

}
