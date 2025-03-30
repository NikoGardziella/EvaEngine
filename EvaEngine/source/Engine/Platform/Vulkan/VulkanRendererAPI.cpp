#include "pch.h"
#include "VulkanRendererAPI.h"
#include <glm/ext/vector_float4.hpp>
#include <Engine/Renderer/VertexArray.h>


namespace Engine {

    void VulkanRendererAPI::Init()
    {
        EE_CORE_INFO("Initializing Vulkan Renderer API");
        // Vulkan setup goes here
    }

    void VulkanRendererAPI::SetViewport(uint32_t x, uint32_t y, uint32_t width, uint32_t height)
    {
        EE_CORE_INFO("Vulkan SetViewport: {}, {}, {}, {}", x, y, width, height);
        // Vulkan viewport setup
    }

    void VulkanRendererAPI::SetClearColor(const glm::vec4& color)
    {
        EE_CORE_INFO("Vulkan SetClearColor: {}, {}, {}, {}", color.r, color.g, color.b, color.a);
    }

    void VulkanRendererAPI::Clear()
    {
        EE_CORE_INFO("Vulkan Clear");
    }

    void VulkanRendererAPI::DrawIndexed(const Ref<VertexArray>& vertexArray, uint32_t indexCount)
    {
        EE_CORE_INFO("Vulkan DrawIndexed: {}", indexCount);
    }

    void VulkanRendererAPI::DrawLines(const Ref<VertexArray>& vertexArray, uint32_t vertexCount)
    {
        EE_CORE_INFO("Vulkan DrawLines: {}", vertexCount);
    }

    void VulkanRendererAPI::DrawIndexedInstanced(const Ref<VertexArray>& vertexArray, uint32_t indexCount, uint32_t instanceCount)
    {
        EE_CORE_INFO("Vulkan DrawIndexedInstanced: {} instances of {}", instanceCount, indexCount);
    }

    void VulkanRendererAPI::SetLineWidth(float thickness)
    {
        EE_CORE_INFO("Vulkan SetLineWidth: {}", thickness);
    }

}