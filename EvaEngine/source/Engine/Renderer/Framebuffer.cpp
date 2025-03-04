#include "pch.h"
#include "Framebuffer.h"
#include "Renderer.h"

#include "Engine/Platform/OpenGl/OpenGLFramebuffer.h"

namespace Engine {


	Ref<Framebuffer> Framebuffer::Create(const FramebufferSpecification& spec)
	{
		switch (Renderer::GetAPI())
		{
		case RendererAPI::API::None:
			EE_CORE_ASSERT(false, "RenderAPI not supported");
			return nullptr;

		case RendererAPI::API::OpenGL:
			return std::make_unique<OpenGLFramebuffer>(spec);

		case RendererAPI::API::Vulkan:
			EE_CORE_ASSERT(false, "Vulkan  not YET supported");

		}
		return nullptr;
	}
}
