#include "pch.h"
#include "Buffer.h"

#include "Renderer.h"
#include <Engine/Core/Core.h>

#include "Engine/Platform/OpenGl/OpenGLBuffer.h"
#include <Engine/Platform/Vulkan/VulkanBuffer.h>

namespace Engine {

	BufferLayout::BufferLayout()
	{

	}
	void BufferLayout::CalculateOffsetAndStride()
	{
		uint32_t offset = 0;
		m_stride;
		for (auto& element : m_elements)
		{
			element.Offset = offset; // Set the current element's offset
			offset += element.Size; // Move the offset forward by the element's size
			m_stride += element.Size;
		}
	}

	Ref<VertexBuffer> VertexBuffer::Create(float* vertices, uint32_t size)
	{

		switch (Renderer::GetAPI())
		{
		case RendererAPI::API::None:
			EE_CORE_ASSERT(false ,"RenderAPI not supported");
			return nullptr;
			
		case RendererAPI::API::OpenGL:
			return std::make_unique<OpenGLVertexBuffer>(vertices, size);

		case RendererAPI::API::Vulkan:
			return std::make_unique<VulkanVertexBuffer>(vertices, size);

			return nullptr;

		
		}
		EE_CORE_ASSERT(false, "RenderAPI unkown");

		return nullptr;
	}

	

	Ref<VertexBuffer> VertexBuffer::Create(uint32_t size)
	{
		switch (Renderer::GetAPI())
		{
		case RendererAPI::API::None:
			EE_CORE_ASSERT(false, "RenderAPI not supported");
			return nullptr;

		case RendererAPI::API::OpenGL:
			return  std::make_unique<OpenGLVertexBuffer>(size);

		case RendererAPI::API::Vulkan:
			EE_CORE_ASSERT(false, "Vulkan  not YET supported");

			return nullptr;


		}
		EE_CORE_ASSERT(false, "RenderAPI unkown");

		return nullptr;
	}

	Ref<IndexBuffer> IndexBuffer::Create(uint32_t* indices, uint32_t count)
	{
		switch (Renderer::GetAPI())
		{
		case RendererAPI::API::None:
			EE_CORE_ASSERT(false, "RenderAPI not supported");
			return nullptr;

		case RendererAPI::API::OpenGL:
			return std::make_unique<OpenGLIndexBuffer>(indices, count);

		case RendererAPI::API::Vulkan:
			EE_CORE_ASSERT(false, "Vulkan  not YET supported");

			return nullptr;


		}
		EE_CORE_ASSERT(false, "RenderAPI unkown");

		return nullptr;
	}

	

}