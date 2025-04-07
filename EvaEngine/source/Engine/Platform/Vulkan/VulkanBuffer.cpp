#include "pch.h"
#include "VulkanBuffer.h"
#include "VulkanContext.h"

namespace Engine {

    namespace BufferUtils {

        void CreateBuffer(uint32_t size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory)
        {
            VulkanContext* context = VulkanContext::Get();
            VkDevice device = context->GetDeviceManager().GetDevice();

            VkBufferCreateInfo bufferInfo{};
            bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
            bufferInfo.size = size;
            bufferInfo.usage = usage;
            bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

            if (vkCreateBuffer(device, &bufferInfo, nullptr, &buffer) != VK_SUCCESS) {
                throw std::runtime_error("Failed to create vertex buffer!");
            }

            VkMemoryRequirements memRequirements;
            vkGetBufferMemoryRequirements(device, buffer, &memRequirements);

            VkMemoryAllocateInfo allocInfo{};
            allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
            allocInfo.allocationSize = memRequirements.size;
            allocInfo.memoryTypeIndex = context->FindMemoryType(memRequirements.memoryTypeBits, properties);

            if (vkAllocateMemory(device, &allocInfo, nullptr, &bufferMemory) != VK_SUCCESS) {
                throw std::runtime_error("Failed to allocate vertex buffer memory!");
            }

            vkBindBufferMemory(device, buffer, bufferMemory, 0);
            EE_CORE_INFO("Vulkan Vertex buffer created");
        }

        void CopyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size)
        {
            VulkanContext* context = VulkanContext::Get();
            VkDevice device = context->GetDeviceManager().GetDevice();

            VkCommandBuffer commandBuffer = context->BeginSingleTimeCommands();

            VkBufferCopy copyRegion{};
            copyRegion.size = size;
            vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);

            context->EndSingleTimeCommands(commandBuffer);
            EE_CORE_INFO("Vulkan Vertex buffer copied");
        }
    }

    VulkanVertexBuffer::VulkanVertexBuffer(float* vertices, uint32_t size)
        : m_size(size)
    {
        VulkanContext* context = VulkanContext::Get();
        VkDevice device = context->GetDeviceManager().GetDevice();

        // Create GPU-only vertex buffer
        BufferUtils::CreateBuffer(size, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, m_buffer, m_bufferMemory);

        // Create temporary staging buffer (CPU-accessible)
        VkBuffer stagingBuffer;
        VkDeviceMemory stagingBufferMemory;
        BufferUtils::CreateBuffer(size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
            stagingBuffer, stagingBufferMemory);

        // Copy vertex data to staging buffer
        void* mappedData;
        vkMapMemory(device, stagingBufferMemory, 0, size, 0, &mappedData);
        memcpy(mappedData, vertices, size);
        vkUnmapMemory(device, stagingBufferMemory);

        // Copy data from staging buffer to GPU buffer
        BufferUtils::CopyBuffer(stagingBuffer, m_buffer, size);

        // Cleanup staging buffer
        vkDestroyBuffer(device, stagingBuffer, nullptr);
        vkFreeMemory(device, stagingBufferMemory, nullptr);
    }

    VulkanVertexBuffer::~VulkanVertexBuffer()
    {
        VkDevice device = VulkanContext::Get()->GetDeviceManager().GetDevice();
        vkDestroyBuffer(device, m_buffer, nullptr);
        vkFreeMemory(device, m_bufferMemory, nullptr);
    }

    

    void VulkanVertexBuffer::Bind() const
    {
        VkCommandBuffer commandBuffer = VulkanContext::Get()->GetCommandBuffer();
        VkDeviceSize offsets[] = { 0 };
        vkCmdBindVertexBuffers(commandBuffer, 0, 1, &m_buffer, offsets);
    }

    void VulkanVertexBuffer::UnBind() const
    {
        VkCommandBuffer commandBuffer = VulkanContext::Get()->GetCommandBuffer();
        VkDeviceSize offsets[] = { 0 };

        // Unbind the vertex buffer by binding nullptr (no buffer)
        vkCmdBindVertexBuffers(commandBuffer, 0, 1, nullptr, offsets);
    }

    void VulkanVertexBuffer::SetData(const void* data, uint32_t size)
    {
        VulkanContext* context = VulkanContext::Get();
        VkDevice device = context->GetDeviceManager().GetDevice();

        // Use a staging buffer instead of direct memory mapping
        VkBuffer stagingBuffer;
        VkDeviceMemory stagingBufferMemory;
        BufferUtils::CreateBuffer(size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
            stagingBuffer, stagingBufferMemory);

        void* mappedData;
        vkMapMemory(device, stagingBufferMemory, 0, size, 0, &mappedData);
        memcpy(mappedData, data, size);
        vkUnmapMemory(device, stagingBufferMemory);

        BufferUtils::CopyBuffer(stagingBuffer, m_buffer, size);

        vkDestroyBuffer(device, stagingBuffer, nullptr);
        vkFreeMemory(device, stagingBufferMemory, nullptr);
    }

    void VulkanVertexBuffer::SetMat4InstanceAttribute(uint32_t location)
    {
    }

    VulkanIndexBuffer::VulkanIndexBuffer(uint32_t* indices, uint32_t count)
    {
        VulkanContext* context = VulkanContext::Get();
        VkDevice device = context->GetDeviceManager().GetDevice();
        VkDeviceSize bufferSize = sizeof(uint32_t) * count;

        // Create GPU-only index buffer
        BufferUtils::CreateBuffer(bufferSize, VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, m_indexBuffer, m_indexBufferMemory);

        // Create staging buffer
        VkBuffer stagingBuffer;
        VkDeviceMemory stagingBufferMemory;
        BufferUtils::CreateBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
            stagingBuffer, stagingBufferMemory);

        // Copy data to staging buffer
        void* mappedData;
        vkMapMemory(device, stagingBufferMemory, 0, bufferSize, 0, &mappedData);
        memcpy(mappedData, indices, (size_t)bufferSize);
        vkUnmapMemory(device, stagingBufferMemory);

        // Copy to GPU buffer
        BufferUtils::CopyBuffer(stagingBuffer, m_indexBuffer, bufferSize);

        vkDestroyBuffer(device, stagingBuffer, nullptr);
        vkFreeMemory(device, stagingBufferMemory, nullptr);
    }

    VulkanIndexBuffer::~VulkanIndexBuffer()
    {
        VkDevice device = VulkanContext::Get()->GetDeviceManager().GetDevice();

        if (m_indexBuffer != VK_NULL_HANDLE)
        {
            vkDestroyBuffer(device, m_indexBuffer, nullptr); 
        }

        if (m_indexBufferMemory != VK_NULL_HANDLE) 
        {
            vkFreeMemory(device, m_indexBufferMemory, nullptr);
        }
    }



    void VulkanIndexBuffer::Bind() const
    {
    }

    void VulkanIndexBuffer::UnBind() const
    {
      
    }


    VulkanBuffer::VulkanBuffer(VkDevice device, VkPhysicalDevice physicalDevice, VkDeviceSize bufferSize, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties)
        : m_device(device), size(bufferSize) {
    {
            // Create Buffer
            VkBufferCreateInfo bufferInfo{};
            bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
            bufferInfo.size = size;
            bufferInfo.usage = usage;
            bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

            if (vkCreateBuffer(device, &bufferInfo, nullptr, &m_buffer) != VK_SUCCESS) {
                throw std::runtime_error("Failed to create buffer!");
            }

            // Allocate Memory
            VkMemoryRequirements memRequirements;
            vkGetBufferMemoryRequirements(device, m_buffer, &memRequirements);

            VkMemoryAllocateInfo allocInfo{};
            allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
            allocInfo.allocationSize = memRequirements.size;
            allocInfo.memoryTypeIndex = FindMemoryType(physicalDevice, memRequirements.memoryTypeBits, properties);

            if (vkAllocateMemory(device, &allocInfo, nullptr, &m_memory) != VK_SUCCESS) {
                throw std::runtime_error("Failed to allocate buffer memory!");
            }

            vkBindBufferMemory(device, m_buffer, m_memory, 0);
        }
    }

    void VulkanBuffer::Destroy()
    {
        vkDestroyBuffer(m_device, m_buffer, nullptr);
        vkFreeMemory(m_device, m_memory, nullptr);
        
    }

    void VulkanBuffer::SetData(const void* data, size_t size)
    {
        EE_CORE_ASSERT(data, "SetData: Data pointer is null");
        EE_CORE_ASSERT(size <= this->size, "SetData: Data size exceeds buffer capacity");

        void* mappedData = nullptr;
        VkResult result = vkMapMemory(m_device, m_memory, 0, size, 0, &mappedData);
        EE_CORE_ASSERT(result == VK_SUCCESS, "Failed to map Vulkan buffer memory");

        std::memcpy(mappedData, data, size);

        // If memory is not host-coherent, i need to flush here
        // im using HOST_COHERENT_BIT, so this is not needed

        vkUnmapMemory(m_device, m_memory);
    }



    uint32_t VulkanBuffer::FindMemoryType(VkPhysicalDevice physicalDevice, uint32_t typeFilter, VkMemoryPropertyFlags properties)
    {
        VkPhysicalDeviceMemoryProperties memProperties;
        vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memProperties);
        for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
            if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties)
            {
                return i;
            }
        }
        throw std::runtime_error("Failed to find suitable memory type!");

    }

}
