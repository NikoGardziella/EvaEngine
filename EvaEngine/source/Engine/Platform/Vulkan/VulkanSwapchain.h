#pragma once

#include "Engine/Platform/Vulkan/VulkanTrackedImage.h"
#include <vulkan/vulkan.h>
#include <vector>

namespace Engine {

    class VulkanSwapchain
    {
    public:
        VulkanSwapchain(VkDevice device, VkSurfaceKHR surface, VkPhysicalDevice physicalDevice);
        ~VulkanSwapchain();

        void RecreateSwapchain();
        void CreateFramebuffers(VkRenderPass renderPass, VkRenderPass imGuiRenderPass, VkRenderPass gameRenderPass, VkDevice device);
        void Cleanup();

        uint32_t FindMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties);

        VkSwapchainKHR GetSwapchain() const { return m_swapchain; }
        VkFormat GetSwapchainImageFormat() const { return m_swapchainImageFormat; }
        VkExtent2D GetSwapchainExtent() const { return m_swapchainExtent; }

        const std::vector<VkImage>& GetSwapchainImages() const { return m_swapchainImages; }
		VkImage GetSwapchainImage(uint32_t index) const { return m_swapchainImages[index]; }

		VkFramebuffer GetSwapchainFramebuffer(uint32_t index) { return m_swapchainFramebuffers[index]; }
		VkFramebuffer GetGameFramebuffer(uint32_t index) const { return m_gameFramebuffers[index]; }
		VkFramebuffer GetImGuiFramebuffer(uint32_t index) const { return m_imguiFramebuffers[index]; }

        VulkanTracked& GetGameTrackedImage(uint32_t imageIndex) { return m_gameTrackedImages[imageIndex]; }
		std::vector<VulkanTracked>& GetGameTrackedImages() { return m_gameTrackedImages; }

    private:

        void CreateSwapchain();
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
        std::vector<VkImage> m_gameImages;

        std::vector<VkImageView> m_swapchainImageViews;
        std::vector<VkImageView> m_imguiImageViews;
        std::vector<VkImageView> m_gameColorAttachmentImageViews;

        std::vector<VkFramebuffer> m_swapchainFramebuffers;
        std::vector<VkFramebuffer> m_imguiFramebuffers;
        std::vector<VkFramebuffer> m_gameFramebuffers;
        std::vector<VkDeviceMemory> m_gameColorAttachmentMemories;

        VkFormat m_swapchainImageFormat;
        VkExtent2D m_swapchainExtent;

        std::vector<VulkanTracked> m_presentTrackedImages;
        std::vector<VulkanTracked> m_gameTrackedImages;
    };

}
