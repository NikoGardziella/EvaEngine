#pragma once
#include <vulkan/vulkan.h>

namespace Engine {
    class VulkanPipelineLayout {
    public:
        VulkanPipelineLayout(VkDevice device);
        ~VulkanPipelineLayout();

        VkPipelineLayout GetPipelineLayout() const { return m_pipelineLayout; }

    private:
        VkDevice m_device;
        VkPipelineLayout m_pipelineLayout;
    };
}
