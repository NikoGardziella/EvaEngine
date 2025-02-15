#include "pch.h"
#include "VertexArray.h"

#include "Engine/Renderer/Renderer.h"
#include "Engine/Platform/OpenGl/OpenGLVertexArray.h"

namespace Engine {



	Ref<VertexArray> VertexArray::Create()
	{

		switch (Renderer::GetAPI())
		{
		case RendererAPI::API::None:
			EE_CORE_ASSERT(false, "RenderAPI not supported");
			return nullptr;

		case RendererAPI::API::OpenGL:
			return std::make_shared<OpenGLVertexArray>();

		case RendererAPI::API::Vulkan:
			EE_CORE_ASSERT(false, "Vulkan  not YET supported");

			return nullptr;


		}
		EE_CORE_ASSERT(false, "RenderAPI unkown");

		return nullptr;
	}

}