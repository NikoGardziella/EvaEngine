#pragma once
#include "vulkan/vulkan.h"
#include <queue>
#include <vector>

namespace Engine {


    class LayerPool {
    public:
        void Init(VkDevice dev, VkImage image, VkFormat format, uint32_t layerCount);
        
        uint32_t Acquire();

        void Release(uint32_t idx) { m_free.push(idx); }
        VkImageView View(uint32_t idx) const { return m_views[idx]; }

        void Destroy();

    private:
        VkDevice m_device = VK_NULL_HANDLE;
        VkImage m_image = VK_NULL_HANDLE;
        VkFormat m_format = VK_FORMAT_UNDEFINED;
        std::vector<VkImageView> m_views;
        std::queue<uint32_t> m_free;
    };


}
