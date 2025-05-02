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

        void CreateSwapchain();
        VkImage CreateImage(VkDevice device, VkPhysicalDevice physicalDevice, uint32_t width, uint32_t height, VkFormat format, VkImageUsageFlags usage, VkDeviceMemory* outMemory = nullptr);
        uint32_t FindMemoryType(VkPhysicalDevice physicalDevice, uint32_t typeFilter, VkMemoryPropertyFlags properties);
        VkImageView CreateImageView(VkDevice device, VkImage image, VkFormat format, VkImageAspectFlags aspectFlags);
        void Cleanup();

        void RecreateSwapchain();

        uint32_t FindMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties);

        VkSwapchainKHR GetSwapchain() const { return m_swapchain; }
        VkFormat GetSwapchainImageFormat() const { return m_swapchainImageFormat; }
        VkExtent2D GetSwapchainExtent() const { return m_swapchainExtent; }
        const std::vector<VkImageView>& GetSwapchainImageViews() const { return m_swapchainImageViews; }
		const std::vector<VkImageView>& GetImGuiImageViews() const { return m_imguiImageViews; }
        const std::vector<VkImage>& GetSwapchainImages() const { return m_swapchainImages; }
		VkImage GetSwapchainImage(uint32_t index) const { return m_swapchainImages[index]; }
		//std::vector<VkFramebuffer> GetSwapchainFramebuffers() const { return m_swapchainFramebuffers; }
		//std::vector<VkFramebuffer> GetImGuiFramebuffers() const { return m_imguiFramebuffers; }
		//std::vector<VkFramebuffer> GetGameFramebuffers() const { return m_gameFramebuffers; }
		VkFramebuffer GetSwapchainFramebuffer(uint32_t index) { return m_swapchainFramebuffers[index]; }
		VkFramebuffer GetGameFramebuffer(uint32_t index) const { return m_gameFramebuffers[index]; }
		VkFramebuffer GetImGuiFramebuffer(uint32_t index) const { return m_imguiFramebuffers[index]; }
		std::vector<VkImageView> GetGameColorAttachmentImageViews() const { return m_gameColorAttachmentImageViews; }
		VkImageView GetGameColorAttachmentImageView(uint32_t index) const { return m_gameColorAttachmentImageViews[index]; }
    
		std::vector<VkImageView> GetGameColorAttachmentImageViews() { return m_gameColorAttachmentImageViews; }
		//std::vector<VkImageView> GetSwapchainImageViews() { return m_swapchainImageViews; }
		std::vector<VkImageView> GetImGuiImageViews() { return m_imguiImageViews; }
		VkImage GetGameImage(uint32_t index) { return m_gameImages[index]; }

		//std::vector<VkImage> GetGameColorAttachments() { return m_gameColorAttachments; }
		//std::vector<VkDeviceMemory> GetGameColorAttachmentMemories() { return m_gameColorAttachmentMemories; }
		VkDeviceMemory GetGameColorAttachmentMemory(uint32_t index) { return m_gameColorAttachmentMemories[index]; }

        VulkanTracked& GetPresentTrackedImage(uint32_t imageIndex) { return m_presentTrackedImages[imageIndex]; }
        VulkanTracked& GetGameTrackedImage(uint32_t imageIndex) { return m_gameTrackedImages[imageIndex]; }
		std::vector<VulkanTracked>& GetGameTrackedImages() { return m_gameTrackedImages; }

        const VulkanTracked& GetPresentTrackedImage(uint32_t imageIndex) const { return m_presentTrackedImages[imageIndex]; }

        void CreateFramebuffers(VkRenderPass renderPass, VkRenderPass imGuiRenderPass, VkRenderPass gameRenderPass, VkDevice device);
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
        std::vector<VkImage> m_gameImages;

        std::vector<VkImageView> m_swapchainImageViews;
        std::vector<VkImageView> m_imguiImageViews;
        std::vector<VkImageView> m_gameColorAttachmentImageViews;
        /*
        *   [Render Game]      --> to --> [Game Framebuffer] (offscreen, sampled by ImGui or post-process)
        *   [Render ImGui]     --> to --> [ImGui Framebuffer] (if using its own)
        *   [Final Output]     --> to --> [Swapchain Framebuffer] (presented on screen)
        */

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
