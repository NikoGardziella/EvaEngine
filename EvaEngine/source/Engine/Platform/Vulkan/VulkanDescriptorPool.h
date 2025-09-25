#pragma once
#include "vulkan/vulkan.h"

namespace Engine {

    class VulkanDescriptorPool {
    public:
        VulkanDescriptorPool(VkDevice device, uint32_t maxSets, const VkDescriptorPoolSize* sizes, uint32_t sizeCount, VkDescriptorPoolCreateFlags flags);
        ~VulkanDescriptorPool();
        void Destroy();
        VkDescriptorPool GetDescriptorPool() const { return m_descriptorPool; }
    private:
        VkDescriptorPool m_descriptorPool = VK_NULL_HANDLE;
        VkDevice m_device = VK_NULL_HANDLE;
    };


}
