#pragma once

#include "vulkan/vulkan.h"

namespace Engine {

    class VulkanDescriptorSet {
    public:
        VulkanDescriptorSet(VkDevice device, VkDescriptorPool pool, VkDescriptorSetLayout layout, VkBuffer uniformBuffer, VkImageView imageView, VkSampler sampler);
        VkDescriptorSet descriptorSet;

    private:
        VkDevice m_device;

    };

}
