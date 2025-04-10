#pragma once
#include "vulkan/vulkan.h"

namespace Engine {

    class VulkanDescriptorPool {
    public:

        VulkanDescriptorPool::VulkanDescriptorPool(VkDevice device, uint32_t maxSets, uint32_t maxUniformBuffers, uint32_t maxCombinedImageSamplers);
		~VulkanDescriptorPool();
        void Destroy();
		VkDescriptorPool GetDescriptorPool() const { return m_descriptorPool; }
    private:
        VkDescriptorPool m_descriptorPool;
        VkDevice m_device;
    };


}
