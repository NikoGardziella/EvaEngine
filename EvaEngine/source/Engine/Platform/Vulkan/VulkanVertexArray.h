#pragma once
#include "Engine/Renderer/VertexArray.h"

#include <vulkan/vulkan.h>
#include <vector>

namespace Engine {

    class VulkanVertexArray : public VertexArray
    {
    public:
        VulkanVertexArray();
        virtual ~VulkanVertexArray();

        virtual void Bind() const override;
        virtual void UnBind() const override;

        virtual void AddVertexBuffer(const Ref<VertexBuffer>& vertexBuffer) override;
        virtual void SetIndexBuffer(const Ref<IndexBuffer>& indexBuffer) override;

        virtual const std::vector<Ref<VertexBuffer>>& GetVertexBuffers() const { return m_vertexBuffers; }
        virtual const Ref<IndexBuffer>& GetIndexBuffer() const { return m_indexBuffer; }


    private:
        std::vector<Ref<VertexBuffer>> m_vertexBuffers;
        Ref<IndexBuffer> m_indexBuffer;

        VkBuffer m_vertexBufferHandle;
        VkBuffer m_indexBufferHandle;

        VkDeviceMemory m_vertexBufferMemory;
        VkDeviceMemory m_indexBufferMemory;

        VkDevice m_device;  
        VkCommandPool m_commandPool;  
        VkQueue m_graphicsQueue;

        VkCommandBuffer m_commandBuffer;

    };




}
