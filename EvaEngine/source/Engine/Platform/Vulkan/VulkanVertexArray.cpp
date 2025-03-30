#include "pch.h"
#include "VulkanVertexArray.h"
#include "VulkanContext.h" 


namespace Engine {


    VulkanVertexArray::VulkanVertexArray()
        : m_vertexBufferHandle(VK_NULL_HANDLE), m_indexBufferHandle(VK_NULL_HANDLE),
        m_vertexBufferMemory(VK_NULL_HANDLE), m_indexBufferMemory(VK_NULL_HANDLE)
    {
        // Obtain the Vulkan device and command pool from the Vulkan context
        VulkanContext* context = VulkanContext::Get();
        m_device = context->GetDevice();
        m_commandPool = context->GetCommandPool();
        m_graphicsQueue = context->GetGraphicsQueue();


        VkCommandBufferAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        allocInfo.commandPool = m_commandPool;
        allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        allocInfo.commandBufferCount = 1;

        VkCommandBuffer commandBuffer;
        vkAllocateCommandBuffers(m_device, &allocInfo, &commandBuffer);


    }

    VulkanVertexArray::~VulkanVertexArray()
    {
        // Clean up Vulkan resources
        if (m_vertexBufferHandle != VK_NULL_HANDLE) {
            vkDestroyBuffer(m_device, m_vertexBufferHandle, nullptr);
            vkFreeMemory(m_device, m_vertexBufferMemory, nullptr);
        }

        if (m_indexBufferHandle != VK_NULL_HANDLE) {
            vkDestroyBuffer(m_device, m_indexBufferHandle, nullptr);
            vkFreeMemory(m_device, m_indexBufferMemory, nullptr);
        }
    }

    void VulkanVertexArray::Bind() const
    {
        // Bind vertex and index buffers for drawing
        VkDeviceSize offsets[] = { 0 };
        vkCmdBindVertexBuffers(m_commandBuffer, 0, 1, &m_vertexBufferHandle, offsets);
        vkCmdBindIndexBuffer(m_commandBuffer, m_indexBufferHandle, 0, VK_INDEX_TYPE_UINT32);
    }

    void VulkanVertexArray::UnBind() const
    {
        // Unbind buffers (Vulkan doesn't have an explicit unbind command, so no op)
    }

    void VulkanVertexArray::AddVertexBuffer(const Ref<VertexBuffer>& vertexBuffer)
    {
        auto context = VulkanContext::Get(); // Ensure context exists
        VkDevice device = context->GetDevice();
        VkPhysicalDevice physicalDevice = context->GetPhysicalDevice();

        if (!device || !physicalDevice)
        {
            throw std::runtime_error("Vulkan device or physical device is not initialized!");
        }

        VkBufferCreateInfo bufferInfo{};
        bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        bufferInfo.size = (uint64_t)vertexBuffer->GetSize(); // Ensure VertexBuffer has a GetSize() method
        bufferInfo.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
        bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        if (vkCreateBuffer(device, &bufferInfo, nullptr, &m_vertexBufferHandle) != VK_SUCCESS)
        {
            throw std::runtime_error("Failed to create vertex buffer!");
        }

        VkMemoryRequirements memRequirements;
        vkGetBufferMemoryRequirements(device, m_vertexBufferHandle, &memRequirements);

        VkMemoryAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        allocInfo.allocationSize = memRequirements.size;
        allocInfo.memoryTypeIndex = context->FindMemoryType(memRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

        if (vkAllocateMemory(device, &allocInfo, nullptr, &m_vertexBufferMemory) != VK_SUCCESS)
        {
            throw std::runtime_error("Failed to allocate vertex buffer memory!");
        }

        vkBindBufferMemory(device, m_vertexBufferHandle, m_vertexBufferMemory, 0);

        m_vertexBuffers.push_back(vertexBuffer);
    }


    void VulkanVertexArray::SetIndexBuffer(const Ref<IndexBuffer>& indexBuffer)
    {
        VulkanContext* context = VulkanContext::Get(); // Ensure VulkanContext has a Get() method
        VkDevice device = context->GetDevice(); // Ensure VulkanContext has GetDevice()

        VkBufferCreateInfo bufferInfo{};
        bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        bufferInfo.size = indexBuffer->GetCount(); // Ensure IndexBuffer has GetSize()
        bufferInfo.usage = VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
        bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        if (vkCreateBuffer(device, &bufferInfo, nullptr, &m_indexBufferHandle) != VK_SUCCESS)
        {
            EE_CORE_ERROR("Failed to create index buffer!");
            return;
        }

        VkMemoryRequirements memRequirements;
        vkGetBufferMemoryRequirements(device, m_indexBufferHandle, &memRequirements);

        VkMemoryAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        allocInfo.allocationSize = memRequirements.size;
        allocInfo.memoryTypeIndex = context->FindMemoryType(memRequirements.memoryTypeBits,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

        if (vkAllocateMemory(device, &allocInfo, nullptr, &m_indexBufferMemory) != VK_SUCCESS)
        {
            EE_CORE_ERROR("Failed to allocate index buffer memory!");
            return;
        }

        vkBindBufferMemory(device, m_indexBufferHandle, m_indexBufferMemory, 0);

        // Copy index data to buffer
        void* data;
        vkMapMemory(device, m_indexBufferMemory, 0, bufferInfo.size, 0, &data);
        memcpy(data, indexBuffer->GetData(), (size_t)bufferInfo.size); // Ensure IndexBuffer has GetData()
        vkUnmapMemory(device, m_indexBufferMemory);

        m_indexBuffer = indexBuffer;
        EE_CORE_INFO("index buffer created. This might be irrelevant");
    }



}