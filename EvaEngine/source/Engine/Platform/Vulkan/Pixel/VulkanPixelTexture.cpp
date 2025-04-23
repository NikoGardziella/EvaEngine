#include "pch.h"

#include "VulkanPixelTexture.h"
#include "Engine/Platform/Vulkan/VulkanContext.h"
#include "Engine/Platform/Vulkan/VulkanUtils.h"
#include <cstring>
#include <Engine/Platform/Vulkan/VulkanBuffer.h>

namespace Engine {

    VulkanPixelTexture::VulkanPixelTexture(const std::string& path)
        : VulkanTexture(path)
    {
        //m_pixelData.resize(m_width * m_height * 4, 0); // Initialize to transparent black
        SetData(m_pixelData.data(), static_cast<uint32_t>(m_pixelData.size()));
        m_solidMask.resize(m_width * m_height, 1); 

        for (int y = 0; y < m_height; ++y)
        {
            for (int x = 0; x < m_width; ++x)
            {
                int index = (y * m_width + x) * 4;
                uint8_t alpha = m_pixelData[index + 3];
                // if the pixel alpha is 0, its not solid
                m_solidMask[y * m_width + x] = (alpha > 0) ? 1 : 0;
            }
        }


    }

    void VulkanPixelTexture::SetPixel(uint32_t x, uint32_t y, uint8_t r, uint8_t g, uint8_t b, uint8_t a)
    {
        if (x >= GetWidth() || y >= GetHeight())
            return;

        size_t index = (y * GetWidth() + x) * 4;
        m_pixelData[index + 0] = r;
        m_pixelData[index + 1] = g;
        m_pixelData[index + 2] = b;
        m_pixelData[index + 3] = a;
    }

    void VulkanPixelTexture::ApplyChanges()
    {
        VulkanDevice& vulkanDeviceManger = VulkanContext::Get()->GetDeviceManager();
        VkDevice device = vulkanDeviceManger.GetDevice();
        VkPhysicalDevice physicalDevice = vulkanDeviceManger.GetPhysicalDevice();

        VulkanBuffer stagingBuffer(
            device,
            physicalDevice,
            static_cast<VkDeviceSize>(m_pixelData.size()),
            VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
        );

        stagingBuffer.SetData(m_pixelData.data(), m_pixelData.size());

        VulkanUtils::TransitionImageLayout(
            m_image, VK_FORMAT_R8G8B8A8_SRGB,
            VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL
        );

        VulkanUtils::CopyBufferToImage(stagingBuffer.GetBuffer(), m_image, GetWidth(), GetHeight());

        VulkanUtils::TransitionImageLayout(
            m_image, VK_FORMAT_R8G8B8A8_SRGB,
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
        );

        stagingBuffer.Destroy();
    }

    bool VulkanPixelTexture::IsPixelSolid(int x, int y) const
    {
        if (x < 0 || x >= m_width || y < 0 || y >= m_height)
            return false;

        return m_solidMask[y * m_width + x] != 0;
    }
    void VulkanPixelTexture::DestroyPixel(int x, int y)
    {
        if (x < 0 || x >= m_width || y < 0 || y >= m_height)
            return;

        m_solidMask[y * m_width + x] = 0;
        m_pixelData[(y * m_width + x) * 4 + 3] = 0; // Set alpha to 0
        
    }
}