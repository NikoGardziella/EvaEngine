#include "pch.h"
#include "VulkanDescriptorPool.h"

namespace Engine {

    VulkanDescriptorPool::VulkanDescriptorPool(VkDevice device, uint32_t maxSets, uint32_t maxUniformBuffers, uint32_t maxCombinedImageSamplers)
        : m_device(device)
    {
        std::array<VkDescriptorPoolSize, 2> poolSizes{};
        poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        poolSizes[0].descriptorCount = maxUniformBuffers;
        poolSizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        poolSizes[1].descriptorCount = maxCombinedImageSamplers;

        VkDescriptorPoolCreateInfo poolInfo{};
        poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
        poolInfo.pPoolSizes = poolSizes.data();
        poolInfo.maxSets = maxSets;

        if (vkCreateDescriptorPool(m_device, &poolInfo, nullptr, &m_descriptorPool) != VK_SUCCESS)
        {
            throw std::runtime_error("failed to create descriptor pool!");
        }
    }

    VulkanDescriptorPool::~VulkanDescriptorPool()
    {
        vkDestroyDescriptorPool(m_device, m_descriptorPool, nullptr);
    }


    void VulkanDescriptorPool::Destroy()
    {
       vkDestroyDescriptorPool(m_device, m_descriptorPool, nullptr);
    }

}
