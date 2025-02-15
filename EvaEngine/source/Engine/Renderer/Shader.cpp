#include "pch.h"
#include "Shader.h"
#include "Renderer.h"

#include "Engine/Platform/OpenGl/OpenGLShader.h"

//#include "glad/glad.h"
#include "Engine/Core/Log.h"
//#include <glm/gtc/type_ptr.hpp>


namespace Engine {

	Ref<Shader> Shader::Create(const std::string& filepath)
	{
		switch (Renderer::GetAPI())
		{
		case RendererAPI::API::None:
			EE_CORE_ASSERT(false, "RenderAPI not supported");
			return nullptr;

		case RendererAPI::API::OpenGL:
			return std::make_shared<OpenGLShader>(filepath);

		case RendererAPI::API::Vulkan:
			EE_CORE_ASSERT(false, "Vulkan  not YET supported");

			return nullptr;


		}
	}

	Ref<Shader> Shader::Create(const std::string& name, const std::string& vertexSource, const std::string& fragmentSource)
	{
		switch (Renderer::GetAPI())
		{
		case RendererAPI::API::None:
			EE_CORE_ASSERT(false, "RenderAPI not supported");
			return nullptr;

		case RendererAPI::API::OpenGL:
			return std::make_shared<OpenGLShader>(name, vertexSource, fragmentSource);

		case RendererAPI::API::Vulkan:
			EE_CORE_ASSERT(false, "Vulkan  not YET supported");

			return nullptr;


		}
		EE_CORE_ASSERT(false, "RenderAPI unkown");

		return nullptr;
	}


	
	void ShaderLibrary::AddShader(const Ref<Shader>& shader)
	{
		auto& name = shader->GetName();

		AddShader(name, shader);
	}

	void ShaderLibrary::AddShader(const std::string& name, const Ref<Shader>& shader)
	{
		EE_CORE_ASSERT(m_shaders.find(name) == m_shaders.end(), "Shader already exist");
		m_shaders[name] = shader;
	}

	Ref<Shader> ShaderLibrary::LoadShader(const std::string& filepath)
	{
		auto shader = Shader::Create(filepath);
		AddShader(shader);
		return shader;
	}

	Ref<Shader> ShaderLibrary::LoadShader(const std::string& name, const std::string& filepath)
	{
		auto shader = Shader::Create(filepath);
		AddShader(name, shader);
		return shader;
	}

	Ref<Shader> ShaderLibrary::GetShader(const std::string& name)
	{
		EE_CORE_ASSERT(m_shaders.find(name) != m_shaders.end(), "Shader not found");

		return m_shaders[name];
	}

}
