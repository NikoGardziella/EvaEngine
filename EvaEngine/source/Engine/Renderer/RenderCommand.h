#pragma once
#include "Engine/Renderer/RendererAPI.h"
#include <Engine/Platform/Vulkan/VulkanRendererAPI.h>
#include "Engine/Platform/OpenGl/OpenGLRendererAPI.h"

namespace Engine {


	class RenderCommand
	{
	public:

		inline static void Init()
		{
			 s_RendererAPI->Init();
		}

		inline static void SetViewport(uint32_t x, uint32_t y, uint32_t width, uint32_t height)
		{
			s_RendererAPI->SetViewport(x, y, width, height);
		}

		inline static void SetClearColor(const glm::vec4& color)
		{
			s_RendererAPI->SetClearColor(color);
		}

		inline static void Clear()
		{
			s_RendererAPI->Clear();
		}

		inline static void DrawIndexed(const Ref<VertexArray>& vertexArray, uint32_t indexCount = 0)
		{
			s_RendererAPI->DrawIndexed(vertexArray, indexCount);
		}

		inline static void DrawIndexedInstanced(const Ref<VertexArray>& vertexArray, uint32_t indexCount, uint32_t instanceCount)
		{
			s_RendererAPI->DrawIndexedInstanced(vertexArray, indexCount, instanceCount);
		}

		inline static void DrawLines(const Ref<VertexArray>& vertexArray, uint32_t vertexCount = 0)
		{
			s_RendererAPI->DrawLines(vertexArray, vertexCount);
		}
		inline static void SetLineWidth(float width)
		{
			s_RendererAPI->SetLineWidth(width);
		}
		
		inline static void SetRendererAPI(RendererAPI::API api)
		{

			switch (api)
			{
			case RendererAPI::API::None:
				EE_CORE_ASSERT(false, "RenderAPI not supported");
				break;

			case RendererAPI::API::OpenGL:
				s_RendererAPI = new OpenGLRendererAPI;
				break;

			case RendererAPI::API::Vulkan:
				s_RendererAPI = new VulkanRendererAPI;
				break;

			default:
				EE_CORE_ASSERT(false, "Unknown RenderAPI");
				break;
			}

		}
		
	private:

		static RendererAPI* s_RendererAPI;
	};

}