#include "pch.h"
#include "LayerPool.h"
#include <Engine/Core/Core.h>
#include <Engine/Core/Assert.h>


namespace Engine {
    void LayerPool::Init(VkDevice dev, VkImage image, VkFormat format, uint32_t layerCount)
    {
        m_device = dev;
        m_image = image;
        m_format = format;
        m_views.resize(layerCount);

        for (uint32_t i = 0; i < layerCount; ++i) {
            VkImageViewCreateInfo vi{};
            vi.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
            vi.pNext = nullptr;
            vi.flags = 0;
            vi.image = m_image;
            vi.viewType = VK_IMAGE_VIEW_TYPE_2D;
            vi.format = m_format;
            vi.components = { VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY,
                              VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY };
            vi.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            vi.subresourceRange.baseMipLevel = 0;
            vi.subresourceRange.levelCount = 1;
            vi.subresourceRange.baseArrayLayer = i;
            vi.subresourceRange.layerCount = 1;

            VkResult r = vkCreateImageView(m_device, &vi, nullptr, &m_views[i]);
            EE_CORE_ASSERT(r == VK_SUCCESS, "LayerPool: failed to create image view (layer %u)");

            m_free.push(i);
        }
    }
    uint32_t LayerPool::Acquire()
    {
        EE_CORE_ASSERT(!m_free.empty(), "LayerPool: out of layers");
        uint32_t idx = m_free.front(); m_free.pop();
        return idx;
    }

    void LayerPool::Destroy()
    {
        for (size_t i = 0; i < m_views.size(); ++i)
        {
            if (m_views[i]) vkDestroyImageView(m_device, m_views[i], nullptr);
        }
        m_views.clear();
        while (!m_free.empty()) m_free.pop();
        m_device = VK_NULL_HANDLE; m_image = VK_NULL_HANDLE;
    }

}