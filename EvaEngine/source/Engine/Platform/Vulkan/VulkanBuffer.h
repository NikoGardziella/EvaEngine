#pragma once
#include "Engine/Renderer/Buffer.h"
#include <vulkan/vulkan.h>

namespace Engine {

    namespace BufferUtils {
        static void CreateBuffer(uint32_t size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory);
        static void CopyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size);

    }

    class VulkanVertexBuffer : public VertexBuffer
    {
    public:
        VulkanVertexBuffer(float* vertices, uint32_t size);
        //VulkanVertexBuffer(uint32_t size);
        virtual ~VulkanVertexBuffer();

        virtual void Bind() const override;
        virtual void UnBind() const override;
        virtual void SetData(const void* data, uint32_t size) override;
        virtual void SetMat4InstanceAttribute(uint32_t location) override;

        virtual void SetLayout(const BufferLayout& layout) override { m_layout = layout; }
        virtual const BufferLayout GetLayout()  const override { return m_layout; }
        virtual uint32_t GetSize() const override { return m_size; }

        VkBuffer GetBuffer() const { return m_buffer; }

    private:
        
        BufferLayout m_layout;

        VkBuffer m_buffer;
        VkDeviceMemory m_bufferMemory;
        VkDevice m_device;

        uint32_t m_size;
    };
    


    class VulkanIndexBuffer : public IndexBuffer
    {
    public:
        VulkanIndexBuffer(uint32_t* indices, uint32_t count);
        virtual ~VulkanIndexBuffer();

        virtual void Bind() const override;
        virtual void UnBind() const override;

        virtual uint32_t GetCount() const override { return m_count; }
        virtual const void* GetData() const override { return m_data.data(); } 
        VkBuffer GetBuffer() const { return m_indexBuffer; }

    private:
        //void CreateIndexBuffer(uint32_t* indices, uint32_t count);

        VkBuffer m_indexBuffer;
        VkDeviceMemory m_indexBufferMemory;
        uint32_t m_count;
        VkDevice m_device;

        std::vector<uint32_t> m_data;
    };

    class VulkanBuffer {
    public:
        VkBuffer m_buffer;
        VkDeviceSize size;

		VulkanBuffer() = default;
        VulkanBuffer(VkDevice device, VkPhysicalDevice physicalDevice, VkDeviceSize bufferSize, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties);
            
		VkBuffer GetBuffer() const { return m_buffer; }
		VkDeviceMemory GetMemory() const { return m_memory; }
        void Destroy();

        void SetData(const void* data, size_t size);

    private:
        VkDeviceMemory m_memory;
        VkDevice m_device;

        uint32_t FindMemoryType(VkPhysicalDevice physicalDevice, uint32_t typeFilter, VkMemoryPropertyFlags properties);
    };



}

