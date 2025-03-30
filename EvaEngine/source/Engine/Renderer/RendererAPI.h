#pragma once

#include "glm/glm.hpp"
#include <Engine/Platform/OpenGl/OpenGLVertexArray.h>

namespace Engine {

	class RendererAPI
	{
	public:

		enum class API
		{
			None = 0, OpenGL = 1, Vulkan
		};

	public:
		virtual ~RendererAPI() = default;

		virtual void Init() = 0;
		virtual void SetViewport(uint32_t x, uint32_t y, uint32_t width, uint32_t height) = 0;
		virtual void SetClearColor(const glm::vec4& color) = 0;
		virtual void Clear() = 0;


		virtual void DrawIndexed(const Ref<VertexArray>& vertexArray, uint32_t indexCount = 0) = 0;
		virtual void DrawLines(const Ref<VertexArray>& vertexArray, uint32_t vertexCount) = 0;
		virtual void DrawIndexedInstanced(const Ref<VertexArray>& vertexArray, uint32_t indexCount, uint32_t instanceCount) = 0;
		virtual void SetLineWidth(float thickness) = 0;

		inline static API GetAPI() { return s_API; }

		inline static void SetRendererAPI(RendererAPI::API api)
		{

			switch (api)
			{
			case RendererAPI::API::None:
				EE_CORE_ASSERT(false, "RenderAPI not supported");
				break;

			case RendererAPI::API::OpenGL:
				s_API = RendererAPI::API::OpenGL;
				break;

			case RendererAPI::API::Vulkan:
				s_API = RendererAPI::API::Vulkan;
				break;

			default:
				EE_CORE_ASSERT(false, "Unknown RenderAPI");
				break;
			}
		}
	private:

		static API s_API;

	private:


	};

}

