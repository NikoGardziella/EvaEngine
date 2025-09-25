#include "pch.h"
#include "VulkanDescriptorPool.h"

namespace Engine {

    VulkanDescriptorPool::VulkanDescriptorPool(VkDevice device, uint32_t maxSets, const VkDescriptorPoolSize* sizes, uint32_t sizeCount, VkDescriptorPoolCreateFlags flags) {
        m_device = device;
        VkDescriptorPoolCreateInfo ci{};
        ci.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        ci.flags = flags;
        ci.maxSets = maxSets;
        ci.poolSizeCount = sizeCount;
        ci.pPoolSizes = sizes;
        VkResult res = vkCreateDescriptorPool(m_device, &ci, nullptr, &m_descriptorPool);
        EE_CORE_ASSERT(res == VK_SUCCESS, "Failed to create descriptor pool");
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
