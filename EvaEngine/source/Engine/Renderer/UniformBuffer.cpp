#include "pch.h"
#include "UniformBuffer.h"

#include "Engine/Renderer/Renderer.h"
#include "Engine/Platform/OpenGl/OpenGLUniformBuffer.h"


namespace Engine {


	Ref<UniformBuffer> UniformBuffer::Create(uint32_t size, uint32_t binding)
	{
		switch (Renderer::GetAPI())
		{
		case RendererAPI::API::None:    EE_CORE_ASSERT(false, "RendererAPI::None is currently not supported!"); return nullptr;
		case RendererAPI::API::OpenGL:  return std::make_shared<OpenGLUniformBuffer>(size, binding);
		}

		EE_CORE_ASSERT(false, "Unknown RendererAPI!");
		return nullptr;
	}
}