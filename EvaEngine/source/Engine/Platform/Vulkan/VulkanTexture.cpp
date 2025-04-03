#include "pch.h"
#include "VulkanTexture.h"
#include "Engine/Platform/Vulkan/VulkanContext.h"
#include "stb_image.h"
#include <stdexcept>

namespace Engine {

    VulkanTexture::VulkanTexture(const std::string& path)
        : m_path(path)
    {
        CreateTextureImage(path);
        CreateTextureImageView();
        CreateTextureSampler();
    }

    VulkanTexture::VulkanTexture(uint32_t width, uint32_t height)
        : m_width(width), m_height(height)
    {
        // Create empty texture image
        // ...
        CreateTextureImageView();
        CreateTextureSampler();
    }

    VulkanTexture::~VulkanTexture()
    {
        VkDevice device = VulkanContext::Get()->GetDeviceManager().GetDevice();
        vkDestroySampler(device, m_sampler, nullptr);
        vkDestroyImageView(device, m_imageView, nullptr);
        vkDestroyImage(device, m_image, nullptr);
        vkFreeMemory(device, m_imageMemory, nullptr);
    }

    void VulkanTexture::Bind(uint32_t slot) const
    {
        // Binding logic here
    }

    void VulkanTexture::SetData(void* data, uint32_t size)
    {
        // Set texture data logic here
    }

    void VulkanTexture::CreateTextureImage(const std::string& path)
    {
        int texWidth, texHeight, texChannels;
        stbi_uc* pixels = stbi_load(path.c_str(), &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
        if (!pixels) {
            throw std::runtime_error("Failed to load texture image!");
        }

        m_width = texWidth;
        m_height = texHeight;
        VkDeviceSize imageSize = m_width * m_height * 4;

        // Create Vulkan image and allocate memory here
        // ...

        stbi_image_free(pixels);
    }

    void VulkanTexture::CreateTextureImageView()
    {
        // Create image view here
        // ...
    }

    void VulkanTexture::CreateTextureSampler()
    {
        // Create sampler here
        // ...
    }

}
