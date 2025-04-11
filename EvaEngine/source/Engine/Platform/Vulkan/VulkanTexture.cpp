#include "pch.h"
#include "VulkanTexture.h"
#include "Engine/Platform/Vulkan/VulkanContext.h"
#include "stb_image.h"
#include <stdexcept>
#include "VulkanUtils.h"
#include "VulkanBuffer.h"
#include <backends/imgui_impl_vulkan.h>
#include "Engine/AssetManager/AssetManager.h"

namespace Engine {

    constexpr VkDeviceSize MAX_TEXTURE_MEMORY_BUDGET = 512 * 1024 * 1024; // 512 MB 

    VulkanTexture::VulkanTexture(const std::string& path, bool imGuiTexture)
        : m_path(path)
    {



        CreateTextureImage(path);
        CreateTextureImageView();
        CreateTextureSampler();

        if (imGuiTexture)
        {
			// m_textureDescriptor is used as TextureId that Imgui uses to bind the texture before imguiDraw
            // if there is no TextureID, imgui will crash at binding.
            // set imGuiTexture to True when adding Imgui texture
            m_textureDescriptor = ImGui_ImplVulkan_AddTexture(m_sampler, m_imageView, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
        }
        AssetManager::s_totalTextureMemory += m_memorySize;
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
        AssetManager::s_totalTextureMemory -= m_memorySize;
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

        if (AssetManager::s_totalTextureMemory + imageSize > MAX_TEXTURE_MEMORY_BUDGET)
        {
			EE_CORE_ASSERT(false, "Texture memory budget exceeded!"); // Handle memory budget exceeded
            // Trigger unloading of unused/least recently used textures
        }


        VkDevice device = VulkanContext::Get()->GetDeviceManager().GetDevice();

        // Create staging buffer
        VulkanBuffer stagingBuffer(
            device,
            VulkanContext::Get()->GetDeviceManager().GetPhysicalDevice(),
            imageSize,
            VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
        );

        // Copy pixel data
        stagingBuffer.SetData(pixels, static_cast<size_t>(imageSize));
        stbi_image_free(pixels);

        // Create the actual Vulkan image
        VulkanUtils::CreateImage(
            m_width,
            m_height,
            VK_FORMAT_R8G8B8A8_SRGB,
            VK_IMAGE_TILING_OPTIMAL,
            VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
            m_image,
            m_imageMemory
        );

        // Transition image layout and copy buffer data
        VulkanUtils::TransitionImageLayout(m_image, VK_FORMAT_R8G8B8A8_SRGB,
            VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

        VulkanUtils::CopyBufferToImage(stagingBuffer.GetBuffer(), m_image, m_width, m_height);

        VulkanUtils::TransitionImageLayout(m_image, VK_FORMAT_R8G8B8A8_SRGB,
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
    }


    void VulkanTexture::CreateTextureImageView()
    {
        VkDevice device = VulkanContext::Get()->GetDeviceManager().GetDevice();

        VkImageViewCreateInfo viewInfo{};
        viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        viewInfo.image = m_image;
        viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        viewInfo.format = VK_FORMAT_R8G8B8A8_SRGB;
        viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        viewInfo.subresourceRange.baseMipLevel = 0;
        viewInfo.subresourceRange.levelCount = 1;
        viewInfo.subresourceRange.baseArrayLayer = 0;
        viewInfo.subresourceRange.layerCount = 1;

        if (vkCreateImageView(device, &viewInfo, nullptr, &m_imageView) != VK_SUCCESS)
        {
			EE_CORE_ASSERT(false, "failed to create texture image view!");
		}

		// Save the memory size for memory usage stats
        VkMemoryRequirements memRequirements;
        vkGetImageMemoryRequirements(device, m_image, &memRequirements);
        m_memorySize = memRequirements.size;
    }


    void VulkanTexture::CreateTextureSampler()
    {
        VkDevice device = VulkanContext::Get()->GetDeviceManager().GetDevice();

        VkSamplerCreateInfo samplerInfo{};
        samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
        samplerInfo.magFilter = VK_FILTER_LINEAR;
        samplerInfo.minFilter = VK_FILTER_LINEAR;
        samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        samplerInfo.anisotropyEnable = VK_FALSE;
        samplerInfo.maxAnisotropy = 16.0f;
        samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
        samplerInfo.unnormalizedCoordinates = VK_FALSE;
        samplerInfo.compareEnable = VK_FALSE;
        samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
        samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
        samplerInfo.mipLodBias = 0.0f;
        samplerInfo.minLod = 0.0f;
        samplerInfo.maxLod = 0.0f;

        if (vkCreateSampler(device, &samplerInfo, nullptr, &m_sampler) != VK_SUCCESS) {
            throw std::runtime_error("failed to create texture sampler!");
        }
    }


}
