#pragma once
#include <vulkan/vulkan.h>
#include <vector>
#include <cstdint>

namespace Engine {

    class Vulkan3DGraphicsPipeline {
    public:
        struct CreateInfo {
            VkDevice                          device = VK_NULL_HANDLE;
            VkRenderPass                      renderPass = VK_NULL_HANDLE;
            VkPipelineCache                   pipelineCache = VK_NULL_HANDLE; // optional
            std::vector<VkDescriptorSetLayout> setLayouts;                    // set=0..N
            VkPushConstantRange               pushConstantRange{};            // optional: size==0 -> ignored
            VkSampleCountFlagBits             msaaSamples = VK_SAMPLE_COUNT_1_BIT;
            uint32_t                          colorAttachmentCount = 1;       // how many color attachments in the subpass
            // Subpass index (usually 0)
            uint32_t                          subpassIndex = 0;
        };

        struct ShaderStages
        {
            VkShaderModule vert = VK_NULL_HANDLE; 
            VkShaderModule frag = VK_NULL_HANDLE;
           
        };

        struct VertexInput {
            std::vector<VkVertexInputBindingDescription>   bindings;
            std::vector<VkVertexInputAttributeDescription> attributes;
        };

        struct RasterState {
            VkPrimitiveTopology topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
            VkPolygonMode       polygonMode = VK_POLYGON_MODE_FILL;
            VkCullModeFlags     cullMode = VK_CULL_MODE_BACK_BIT;
            VkFrontFace         frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
            VkBool32            depthTest = VK_TRUE;
            VkBool32            depthWrite = VK_TRUE;
            VkCompareOp         depthCompare = VK_COMPARE_OP_LESS_OR_EQUAL;
            VkBool32            enableBlending = VK_FALSE; // if true: srcAlpha/oneMinusSrcAlpha
        };

    public:
        Vulkan3DGraphicsPipeline() = default;
        ~Vulkan3DGraphicsPipeline() { Destroy(); }


        

        // Create the pipeline + layout
        bool Create(const CreateInfo& ci,  const ShaderStages& stages, const VertexInput& vi, const RasterState& rs);

        // Destroy pipeline + layout (safe if not created)
        void Destroy();

        // Bind pipeline to a command buffer
        void Bind(VkCommandBuffer cmd) const;

        // Accessors
        VkPipeline       Get()        const { return m_pipeline; }
        VkPipelineLayout GetLayout()  const { return m_layout; }
        VkDevice         GetDevice()  const { return m_device; }

    private:
        VkDevice        m_device = VK_NULL_HANDLE;
        VkPipeline      m_pipeline = VK_NULL_HANDLE;
        VkPipelineLayout m_layout = VK_NULL_HANDLE;

        // cached for potential recreation (optional, not used here)
        CreateInfo      m_ci{};
    };

} // namespace Engine
