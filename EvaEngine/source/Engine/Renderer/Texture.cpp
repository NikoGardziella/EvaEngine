#include "pch.h"
#include "Texture.h"

#include "Engine/Renderer/Renderer.h"
#include "Engine/Platform/OpenGl/OpenGLTexture.h"

namespace Engine {



	Ref<Texture2D> Texture2D::Create(const std::string& path)
	{
		switch (Renderer::GetAPI())
		{
		case RendererAPI::API::None:
			EE_CORE_ASSERT(false, "RenderAPI not supported");
			return nullptr;

		case RendererAPI::API::OpenGL:
			return std::make_shared<OpenGLTexture2D>(path);

		case RendererAPI::API::Vulkan:
			EE_CORE_ASSERT(false, "Vulkan  not YET supported");

			return nullptr;


		}
		EE_CORE_ASSERT(false, "RenderAPI unkown");

		return nullptr;
	}


	Ref<Texture2D> Texture2D::Create(uint32_t width, uint32_t height)
	{
		switch (Renderer::GetAPI())
		{
		case RendererAPI::API::None:
			EE_CORE_ASSERT(false, "RenderAPI not supported");
			return nullptr;

		case RendererAPI::API::OpenGL:
			return std::make_shared<OpenGLTexture2D>(width, height);

		case RendererAPI::API::Vulkan:
			EE_CORE_ASSERT(false, "Vulkan  not YET supported");

			return nullptr;


		}
		EE_CORE_ASSERT(false, "RenderAPI unkown");

		return nullptr;
	}
}