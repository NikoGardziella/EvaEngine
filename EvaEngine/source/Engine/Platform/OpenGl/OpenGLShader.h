#pragma once

#include "Engine/Renderer/Shader.h"
#include "glm/glm.hpp"


//#include <glad/glad.h>
typedef unsigned int GLenum;


namespace Engine {

	class OpenGLShader : public Shader
	{

	public:

		OpenGLShader(const std::string& name, const std::string& vertexSource, const std::string& fragmentSource);
		OpenGLShader(const std::string& filepath);
		virtual ~OpenGLShader();

		
		virtual void Bind() const override;
		virtual void Unbind() const override;
		virtual const std::string& GetName() const override { return m_name; }

		virtual void SetMat4(const std::string& name, const glm::mat4& value) override;
		virtual void SetFloat4(const std::string& name, const glm::vec4& value) override;
		virtual void SetFloat3(const std::string& name, const glm::vec3& value) override;
		virtual void SetFloat(const std::string& name, float value) override;
		virtual void SetInt(const std::string& name, int value) override;
		virtual void SetIntArray(const std::string& name, int* values, uint32_t count) override;

		void UploadUniformFloat4(const std::string& name, const glm::vec4& values);
		void UploadUniformFloat3(const std::string& name, const glm::vec3& value);
		void UploadUniformFloat2(const std::string& name, const glm::vec2& value);
		void UploadUniformFloat(const std::string& name, float value);

		void UploadUniformInt(const std::string& name, int value);
		void UploadUniformIntArray(const std::string& name, int* values, uint32_t count);

		void UploadUniformMat3(const std::string& name, const glm::mat3& matrix);
		void UploadUniformMat4(const std::string& name, const glm::mat4& matrix);

		

	private:

		void Compile(const std::unordered_map<GLenum, std::string>& source);
		std::unordered_map<GLenum, std::string> PreProcess(const std::string& source);
		std::string ReadFile(const std::string& filepath);

	private:

		uint32_t m_renderID;
		std::string m_name;
	

	};

}

