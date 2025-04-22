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
        VulkanContext* vulkaContext = VulkanContext::Get();
        VkDevice device = vulkaContext->GetDeviceManager().GetDevice();
        VkPhysicalDevice physicalDevice = vulkaContext->GetDeviceManager().GetPhysicalDevice();

        // 1. Create Image
        VkImageCreateInfo imageInfo{};
        imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        imageInfo.imageType = VK_IMAGE_TYPE_2D;
        imageInfo.extent.width = width;
        imageInfo.extent.height = height;
        imageInfo.extent.depth = 1;
        imageInfo.mipLevels = 1;
        imageInfo.arrayLayers = 1;
        imageInfo.format = VK_FORMAT_R8G8B8A8_SRGB;
        imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
        imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        imageInfo.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
        imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
        imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        if (vkCreateImage(device, &imageInfo, nullptr, &m_image) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create image!");
        }

        // 2. Allocate memory and bind
        VkMemoryRequirements memRequirements;
        vkGetImageMemoryRequirements(device, m_image, &memRequirements);

        VkMemoryAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        allocInfo.allocationSize = memRequirements.size;
        allocInfo.memoryTypeIndex = vulkaContext->FindMemoryType(
            memRequirements.memoryTypeBits,
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
        );

        if (vkAllocateMemory(device, &allocInfo, nullptr, &m_imageMemory) != VK_SUCCESS) {
            throw std::runtime_error("Failed to allocate image memory!");
        }

        vkBindImageMemory(device, m_image, m_imageMemory, 0);

        // 3. Create image view and sampler
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
        if (!pixels)
        {
            EE_CORE_ERROR("Failed to load texture image!");
        }

        m_width = texWidth;
        m_height = texHeight;
        VkDeviceSize imageSize = m_width * m_height * 4;

        if (AssetManager::s_totalTextureMemory + imageSize > MAX_TEXTURE_MEMORY_BUDGET)
        {
			EE_CORE_ASSERT(false, "Texture memory budget exceeded!");
            // unloading of unused/least recently used textures
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
        m_pixelData.resize(m_width * m_height * 4);
        memcpy(m_pixelData.data(), pixels, m_pixelData.size());
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
        samplerInfo.magFilter = VK_FILTER_NEAREST; // for pixel graphics. VK_FILTER_LINEAR will interpolate
        samplerInfo.minFilter = VK_FILTER_NEAREST;
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

        if (vkCreateSampler(device, &samplerInfo, nullptr, &m_sampler) != VK_SUCCESS)
        {
            EE_CORE_ERROR("failed to create texture sampler!");
        }
    }


}
