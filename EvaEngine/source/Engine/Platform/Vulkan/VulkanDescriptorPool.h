#pragma once
#include "vulkan/vulkan.h"

namespace Engine {

    class VulkanDescriptorPool {
    public:
        VkDescriptorPool m_descriptorPool;

        VulkanDescriptorPool::VulkanDescriptorPool(VkDevice device, uint32_t maxSets, uint32_t maxUniformBuffers, uint32_t maxCombinedImageSamplers);
		~VulkanDescriptorPool();
        void Destroy();

    private:
        VkDevice m_device;
    };


}
