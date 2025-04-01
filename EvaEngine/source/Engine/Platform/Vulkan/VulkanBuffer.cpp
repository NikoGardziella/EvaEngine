#include "pch.h"
#include "VulkanBuffer.h"
#include "VulkanContext.h"

namespace Engine {

	VulkanVertexBuffer::VulkanVertexBuffer(float* vertices, uint32_t size)
		: m_size(size)
	{
		VulkanContext* context = VulkanContext::Get();
		//VkDevice device = context->GetDevice();
		VkDevice device;

		CreateBuffer(size, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, m_buffer, m_bufferMemory);

		// Create a temporary staging buffer (CPU accessible)
		VkBuffer stagingBuffer;
		VkDeviceMemory stagingBufferMemory;
		CreateBuffer(size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
			stagingBuffer, stagingBufferMemory);

		// Copy vertex data to staging buffer
		void* mappedData;
		vkMapMemory(device, stagingBufferMemory, 0, size, 0, &mappedData);
		memcpy(mappedData, vertices, size);
		vkUnmapMemory(device, stagingBufferMemory);

		// Copy data from staging buffer to GPU buffer
		CopyBuffer(stagingBuffer, m_buffer, size);

		// Cleanup staging buffer
		vkDestroyBuffer(device, stagingBuffer, nullptr);
		vkFreeMemory(device, stagingBufferMemory, nullptr);
	}

	void VulkanVertexBuffer::CreateBuffer(uint32_t size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory)
	{
		VulkanContext* context = VulkanContext::Get();
		//VkDevice device = context->GetDevice();
		VkDevice device;

		VkBufferCreateInfo bufferInfo{};
		bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		bufferInfo.size = size;
		bufferInfo.usage = usage;
		bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

		if (vkCreateBuffer(device, &bufferInfo, nullptr, &buffer) != VK_SUCCESS)
		{
			throw std::runtime_error("Failed to create vertex buffer!");
		}

		VkMemoryRequirements memRequirements;
		vkGetBufferMemoryRequirements(device, buffer, &memRequirements);

		VkMemoryAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		allocInfo.allocationSize = memRequirements.size;
		allocInfo.memoryTypeIndex = context->FindMemoryType(memRequirements.memoryTypeBits, properties);

		if (vkAllocateMemory(device, &allocInfo, nullptr, &bufferMemory) != VK_SUCCESS)
		{
			throw std::runtime_error("Failed to allocate vertex buffer memory!");
		}

		vkBindBufferMemory(device, buffer, bufferMemory, 0);
		EE_CORE_INFO("Vulkan Vertex buffer created");
	}

	void VulkanVertexBuffer::CopyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size)
	{
		VulkanContext* context = VulkanContext::Get();
		//VkDevice device = context->GetDevice();
		VkDevice device;

		VkCommandBuffer commandBuffer = context->BeginSingleTimeCommands();

		VkBufferCopy copyRegion{};
		copyRegion.size = size;
		vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);

		context->EndSingleTimeCommands(commandBuffer);
		EE_CORE_INFO("Vulkan Vertex buffer copied");

	}

	VulkanVertexBuffer::VulkanVertexBuffer(uint32_t size)
	{

	}

	VulkanVertexBuffer::~VulkanVertexBuffer()
	{
		//VkDevice device = VulkanContext::Get()->GetDevice();
		VkDevice device;

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
	}
	void VulkanVertexBuffer::SetData(const void* data, uint32_t size)
	{
		VulkanContext* context = VulkanContext::Get();
		//VkDevice device = context->GetDevice();
		VkDevice device;

		void* mappedData;
		vkMapMemory(device, m_bufferMemory, 0, size, 0, &mappedData);
		memcpy(mappedData, data, size);
		vkUnmapMemory(device, m_bufferMemory);
	}

	


	void VulkanVertexBuffer::SetMat4InstanceAttribute(uint32_t location)
	{
	}



	VulkanIndexBuffer::VulkanIndexBuffer(uint32_t* indices, uint32_t count)
		: m_count(count), m_data(indices, indices + count)
	{
		CreateIndexBuffer(indices, count);
	}

	VulkanIndexBuffer::~VulkanIndexBuffer()
	{
		VulkanContext* context = VulkanContext::Get();
		//VkDevice device = context->GetDevice();
		VkDevice device;

		vkDestroyBuffer(device, m_indexBuffer, nullptr);
		vkFreeMemory(device, m_indexBufferMemory, nullptr);
	}

	void VulkanIndexBuffer::CreateIndexBuffer(uint32_t* indices, uint32_t count)
	{
		VulkanContext* context = VulkanContext::Get();
		VkDevice device;
		//VkDevice device = context->GetDevice();

		VkDeviceSize bufferSize = sizeof(uint32_t) * count;

		// Create buffer
		VkBufferCreateInfo bufferInfo{};
		bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		bufferInfo.size = bufferSize;
		bufferInfo.usage = VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
		bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

		if (vkCreateBuffer(device, &bufferInfo, nullptr, &m_indexBuffer) != VK_SUCCESS)
		{
			EE_CORE_ERROR("Failed to create index buffer!");
			return;
		}

		// Allocate memory
		VkMemoryRequirements memRequirements;
		vkGetBufferMemoryRequirements(device, m_indexBuffer, &memRequirements);

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

		vkBindBufferMemory(device, m_indexBuffer, m_indexBufferMemory, 0);

		// Copy index data into buffer
		void* data;
		vkMapMemory(device, m_indexBufferMemory, 0, bufferSize, 0, &data);
		memcpy(data, indices, (size_t)bufferSize);
		vkUnmapMemory(device, m_indexBufferMemory);
	}

	void VulkanIndexBuffer::Bind() const
	{
		VulkanContext* context = VulkanContext::Get();
		VkCommandBuffer commandBuffer = context->GetCommandBuffer(); // Ensure you have a function for this

		vkCmdBindIndexBuffer(commandBuffer, m_indexBuffer, 0, VK_INDEX_TYPE_UINT32);
	}




}