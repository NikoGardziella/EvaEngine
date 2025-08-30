#include "pch.h"
#include "VulkanTexture.h"
#include "Engine/Platform/Vulkan/VulkanContext.h"
#include "stb_image.h"
#include "VulkanUtils.h"
#include "VulkanBuffer.h"
#include <backends/imgui_impl_vulkan.h>
#include "Engine/AssetManager/AssetManager.h"

namespace Engine {

    constexpr VkDeviceSize MAX_TEXTURE_MEMORY_BUDGET = 512 * 1024 * 1024; // 512 MB 

    VulkanTexture::VulkanTexture(const std::string& path, VkFormat textureFormat,const std::string& name, bool imGuiTexture, uint32_t textureID)
		: m_path(path), m_name(name), m_TextureID(textureID), m_textureFormat(textureFormat)
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

    VulkanTexture::VulkanTexture(uint32_t width, uint32_t height, VkFormat textureFormat, bool imGuiTexture, uint32_t textureID)
		: m_width(width), m_height(height), m_TextureID(textureID), m_textureFormat(textureFormat)
    {


        CreateTextureImage();
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
        EE_CORE_ASSERT(data, "SetData called with null data");
      
        //EE_CORE_ASSERT(size == m_width * m_height * 4, "Staging buffer size mismatch");

        VulkanContext* vulkaContext = VulkanContext::Get();

        VkDevice device = vulkaContext->GetDeviceManager().GetDevice();
        VkQueue& graphicsQueue = vulkaContext->GetGraphicsQueue();
        VkCommandPool& cmdPool = vulkaContext->GetCommandPool();

        VulkanBuffer stagingBuffer(
            device,
            VulkanContext::Get()->GetDeviceManager().GetPhysicalDevice(),
            size,
            VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
        );
        uint8_t* src = reinterpret_cast<uint8_t*>(data);

       


        void* mapped;
        vkMapMemory(device, stagingBuffer.GetMemory(), 0, size, 0, &mapped);
        std::memcpy(mapped, data, size);
        vkUnmapMemory(device, stagingBuffer.GetMemory());
        
        
        VkCommandBuffer cmd = vulkaContext->BeginSingleTimeCommands();
        EE_CORE_ASSERT(cmd != VK_NULL_HANDLE, "Command buffer is null");

        VulkanUtils::TransitionImageLayout(cmd,
            m_image, m_textureFormat,
            VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);


        EE_CORE_ASSERT(stagingBuffer.GetBuffer() != VK_NULL_HANDLE, "Staging buffer is null");

        VulkanUtils::CopyBufferToImage(cmd,
            stagingBuffer.GetBuffer(), m_image, m_width, m_height);

        VulkanUtils::TransitionImageLayout( cmd,
            m_image, m_textureFormat,
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

        vulkaContext->EndSingleTimeCommands(cmd);


    }




    void VulkanTexture::CreateTextureImage()
    {

        VulkanContext* vulkaContext = VulkanContext::Get();
        VkDevice device = vulkaContext->GetDeviceManager().GetDevice();
        VkPhysicalDevice physicalDevice = vulkaContext->GetDeviceManager().GetPhysicalDevice();


        // 1. Create Image
        VkImageCreateInfo imageInfo{};
        imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        imageInfo.imageType = VK_IMAGE_TYPE_2D;
        imageInfo.extent.width = m_width;
        imageInfo.extent.height = m_height;
        imageInfo.extent.depth = 1;
        imageInfo.mipLevels = 1;
        imageInfo.arrayLayers = 1;
        imageInfo.format = m_textureFormat;
        imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
        imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        imageInfo.usage = VK_IMAGE_USAGE_STORAGE_BIT // for compute writes
            | VK_IMAGE_USAGE_SAMPLED_BIT       // for  fragment shader
            | VK_IMAGE_USAGE_TRANSFER_DST_BIT
            | VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
        imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
        imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        if (vkCreateImage(device, &imageInfo, nullptr, &m_image) != VK_SUCCESS)
        {
            EE_CORE_ASSERT(false, "Failed to create image!");
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

        if (vkAllocateMemory(device, &allocInfo, nullptr, &m_imageMemory) != VK_SUCCESS)
        {
            EE_CORE_ASSERT(false, "Failed to allocate image memory!");
        }

        vkBindImageMemory(device, m_image, m_imageMemory, 0);
    }

    void VulkanTexture::CreateTextureImage(const std::string& path)
    {
        int texWidth, texHeight, texChannels;
        //stbi_set_flip_vertically_on_load(true);
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
            VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
        );

        // Copy pixel data
        stagingBuffer.SetData(pixels, static_cast<size_t>(imageSize));
        m_CPUpixelData.resize(m_width * m_height * 4);
        memcpy(m_CPUpixelData.data(), pixels, m_CPUpixelData.size());
        stbi_image_free(pixels);

        // Create the actual Vulkan image
        VulkanUtils::CreateImage(
            m_width,
            m_height,
            m_textureFormat,
            VK_IMAGE_TILING_OPTIMAL,
            VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT
            | VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT,
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
            m_image,
            m_imageMemory
        );

        // Transition image layout and copy buffer data
        VulkanUtils::TransitionImageLayout(m_image, m_textureFormat,
            VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

        VulkanUtils::CopyBufferToImage(stagingBuffer.GetBuffer(), m_image, m_width, m_height);

        VulkanUtils::TransitionImageLayout(m_image, m_textureFormat,
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

        m_CurrentLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

    }


    void VulkanTexture::CreateTextureImageView()
    {
        VkDevice device = VulkanContext::Get()->GetDeviceManager().GetDevice();

        VkImageViewCreateInfo viewInfo{};
        viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        viewInfo.image = m_image;
        viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        viewInfo.format = m_textureFormat;
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

        VkSamplerCreateInfo si{};
        si.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
        si.magFilter = VK_FILTER_NEAREST;                    // no interpolation
        si.minFilter = VK_FILTER_NEAREST;
        si.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;       // important: no mip blending
        si.minLod = 0.0f;                                 // clamp to base level
        si.maxLod = 0.0f;

        si.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE; // do NOT use REPEAT for baked chunks
        si.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        si.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;

        si.anisotropyEnable = VK_FALSE;                      // off for pixel-perfect
        si.maxAnisotropy = 1.0f;                          // ignored when disabled
        si.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
        si.unnormalizedCoordinates = VK_FALSE;
        si.compareEnable = VK_FALSE;
        si.compareOp = VK_COMPARE_OP_ALWAYS;

        if (vkCreateSampler(device, &si, nullptr, &m_sampler) != VK_SUCCESS) {
            EE_CORE_ERROR("failed to create chunk texture sampler!");
        }
    }


    Ref<VulkanTexture> VulkanTexture::Clone() const
    {
        auto clone = std::make_shared<VulkanTexture>(m_width, m_height, VK_FORMAT_R8G8B8A8_UNORM);

        clone->CopyFrom(*this);
        clone->SetTextureID(this->GetTextureID());
        clone->SetName(this->GetName());
        clone->SetCheckCollision(this->GetCheckCollision());
		clone->SetCurrentLayout(this->GetCurrentLayout());
        clone->SetHeight(this->GetHeight());
		clone->SetWidth(this->GetWidth());

        return clone;
    }

   


    void VulkanTexture::CopyFrom(const VulkanTexture& src)
    {

        VulkanContext* vulkaContext = VulkanContext::Get();
        VkDevice device = vulkaContext->GetDeviceManager().GetDevice();
        VkCommandPool transferPool = vulkaContext->GetCommandPool();
        VkQueue transferQueue = vulkaContext->GetGraphicsQueue();
        // 1) Allocate a one-time command buffer
        VkCommandBufferAllocateInfo allocInfo{ VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO };
        allocInfo.commandPool = transferPool;
        allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        allocInfo.commandBufferCount = 1;

        VkCommandBuffer cmd;
        vkAllocateCommandBuffers(device, &allocInfo, &cmd);

        VkCommandBufferBeginInfo beginInfo{ VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
        beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
        vkBeginCommandBuffer(cmd, &beginInfo);

        // 2) Transition src  TRANSFER_SRC_OPTIMAL
        {
            VkImageMemoryBarrier barrier{ VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER };
            barrier.oldLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;  // or GENERAL
            barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
            barrier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT;
            barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
            barrier.image = src.m_image;
            barrier.subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };

            vkCmdPipelineBarrier(
                cmd,
                VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
                VK_PIPELINE_STAGE_TRANSFER_BIT,
                0,
                0, nullptr,
                0, nullptr,
                1, &barrier
            );
        }

        // 3) Transition dst (this)  TRANSFER_DST_OPTIMAL
        {
            VkImageMemoryBarrier barrier{ VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER };
            barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
            barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
            barrier.srcAccessMask = 0;
            barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
            barrier.image = m_image;
            barrier.subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };

            vkCmdPipelineBarrier(
                cmd,
                VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                VK_PIPELINE_STAGE_TRANSFER_BIT,
                0,
                0, nullptr,
                0, nullptr,
                1, &barrier
            );
        }

        // 4) Copy the image
        {
            VkImageCopy copyRegion{};
            copyRegion.srcSubresource = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1 };
            copyRegion.dstSubresource = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1 };
            copyRegion.extent = { m_width, m_height, 1 };

            vkCmdCopyImage(
                cmd,
                src.m_image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                m_image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                1, &copyRegion
            );
        }

        {
            VkImageMemoryBarrier barrier{ VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER };
            barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
            barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
            barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
            barrier.image = m_image;
            barrier.subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };

            vkCmdPipelineBarrier(
                cmd,
                VK_PIPELINE_STAGE_TRANSFER_BIT,
                VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
                0,
                0, nullptr,
                0, nullptr,
                1, &barrier
            );


        }

        // 6) Transition src back to its original layout (if needed)
        {
            VkImageMemoryBarrier barrier{ VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER };
            barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
            barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL; // or GENERAL
            barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
            barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
            barrier.image = src.m_image;
            barrier.subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };

            vkCmdPipelineBarrier(
                cmd,
                VK_PIPELINE_STAGE_TRANSFER_BIT,
                VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
                0,
                0, nullptr,
                0, nullptr,
                1, &barrier
            );


        }

        // 7) End & submit, then wait
        vkEndCommandBuffer(cmd);

        VkSubmitInfo submit{ VK_STRUCTURE_TYPE_SUBMIT_INFO };
        submit.commandBufferCount = 1;
        submit.pCommandBuffers = &cmd;
        vkQueueSubmit(transferQueue, 1, &submit, VK_NULL_HANDLE);
        vkQueueWaitIdle(transferQueue);

        vkFreeCommandBuffers(device, transferPool, 1, &cmd);

    }




}
