#include "pch.h"
#include "OpenGLShader.h"

#include <fstream>

#include "Engine/Core/Log.h"
#include <glm/gtc/type_ptr.hpp>
#include <glad/glad.h>

namespace Engine {

	static GLenum ShaderTypeFromString(const std::string& type)
	{
		if (type == "vertex")
			return GL_VERTEX_SHADER;
		if (type == "fragment" || type == "pixel")
			return GL_FRAGMENT_SHADER;

		EE_CORE_ASSERT(false, "Unknown shader type!");
		return 0;
	}

	OpenGLShader::OpenGLShader(const std::string& filepath)
	{
		EE_PROFILE_FUNCTION();

		std::string source = ReadFile(filepath);
		auto shaderSources = PreProcess(source);
		Compile(shaderSources);

		// assets/shaders/shaderm.glsl
		// Extract name from filepath
		/*
		auto lastSlash = filepath.find_last_of("/\\");
		lastSlash = lastSlash == std::string::npos ? 0 : lastSlash + 1;
		auto lastDot = filepath.rfind('.');
		auto count = lastDot == std::string::npos ? filepath.size() - lastSlash : lastDot - lastSlash;
		m_name = filepath.substr(lastSlash, count);
		*/
		m_name = std::filesystem::path(filepath).filename().string();
	}

	OpenGLShader::OpenGLShader(const std::string& name, const std::string& vertexSource, const std::string& fragmentSource)
		: m_name(name)
	{
		EE_PROFILE_FUNCTION();

		std::unordered_map<GLenum, std::string> sources;

		sources[GL_VERTEX_SHADER] = vertexSource;
		sources[GL_FRAGMENT_SHADER] = fragmentSource;
		Compile(sources);

		
	}


	OpenGLShader::~OpenGLShader()
	{
		glDeleteProgram(m_renderID);
	}

	void OpenGLShader::Bind() const
	{
		EE_PROFILE_FUNCTION();

		glUseProgram(m_renderID);
	}

	void OpenGLShader::Unbind() const
	{
		EE_PROFILE_FUNCTION();

		glUseProgram(0);
	}

	void OpenGLShader::SetMat4(const std::string& name, const glm::mat4& value)
	{
		EE_PROFILE_FUNCTION();

		UploadUniformMat4(name, value);

	}

	void OpenGLShader::SetFloat4(const std::string& name, const glm::vec4& value)
	{
		EE_PROFILE_FUNCTION();

		UploadUniformFloat4(name, value);

	}

	void OpenGLShader::SetFloat3(const std::string& name,const glm::vec3& value)
	{
		EE_PROFILE_FUNCTION();

		UploadUniformFloat3(name, value);
	}

	void OpenGLShader::SetFloat(const std::string& name, float value)
	{
		EE_PROFILE_FUNCTION();

		UploadUniformFloat(name, value);
	}

	void OpenGLShader::SetInt(const std::string& name, int value)
	{
		EE_PROFILE_FUNCTION();

		UploadUniformInt(name, value);
	}

	void OpenGLShader::SetIntArray(const std::string& name, int* values, uint32_t count)
	{
		EE_PROFILE_FUNCTION();

		UploadUniformIntArray(name, values, count);
	}


	void OpenGLShader::UploadUniformFloat4(const std::string& name, const glm::vec4& values)
	{
		GLint location = glGetUniformLocation(m_renderID, name.c_str());
		glUniform4f(location, values.x, values.y, values.z, values.w);
	}

	void OpenGLShader::UploadUniformFloat3(const std::string& name, const glm::vec3& value)
	{
		GLint location = glGetUniformLocation(m_renderID, name.c_str());

		glUniform3f(location, value.x, value.y, value.z);
	}

	void OpenGLShader::UploadUniformFloat2(const std::string& name, const glm::vec2& value)
	{
		GLint location = glGetUniformLocation(m_renderID, name.c_str());

		glUniform2f(location, value.x, value.y);
	}

	void OpenGLShader::UploadUniformFloat(const std::string& name, float value)
	{
		GLint location = glGetUniformLocation(m_renderID, name.c_str());

		glUniform1f(location, value);
	}

	void OpenGLShader::UploadUniformInt(const std::string& name, int value)
	{
		GLint location = glGetUniformLocation(m_renderID, name.c_str());

		glUniform1i(location, value);
	}

	void OpenGLShader::UploadUniformIntArray(const std::string& name, int* values, uint32_t count)
	{
		GLint location = glGetUniformLocation(m_renderID, name.c_str());

		glUniform1iv(location, count, values);
	}

	void OpenGLShader::UploadUniformMat3(const std::string& name, const glm::mat3& matrix)
	{
		GLint location = glGetUniformLocation(m_renderID, name.c_str());
		glUniformMatrix3fv(location, 1, GL_FALSE, glm::value_ptr(matrix));
	}

	void OpenGLShader::UploadUniformMat4(const std::string& name, const glm::mat4& matrix)
	{
		GLint location = glGetUniformLocation(m_renderID, name.c_str());

		// f = float, v = array of floats
		// value_ptr = pointer to the internal raw data in memory
		glUniformMatrix4fv(location, 1, GL_FALSE, glm::value_ptr(matrix));

	}


	void OpenGLShader::Compile(const std::unordered_map<GLenum, std::string>& shaderSources)
	{
		EE_PROFILE_FUNCTION();

		// Create program
		GLuint program = glCreateProgram();
		EE_CORE_ASSERT(shaderSources.size() >= 2, "max shader count is 2");

		std::array<GLuint, 2> shaderIDs; // Holds valid shader IDs

		int glShaderIDIndex = 0;
		for (const auto& kv : shaderSources) {
			GLenum shaderType = kv.first;
			const std::string& shaderSource = kv.second;

			// Create and compile shader
			GLuint shader = glCreateShader(shaderType);
			const GLchar* source = shaderSource.c_str();
			glShaderSource(shader, 1, &source, nullptr);
			glCompileShader(shader);

			// Check compilation status
			GLint isCompiled = 0;
			glGetShaderiv(shader, GL_COMPILE_STATUS, &isCompiled);
			if (isCompiled == GL_FALSE) {
				// Get and log compilation error
				GLint maxLength = 0;
				glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &maxLength);
				std::vector<GLchar> infoLog(maxLength);
				glGetShaderInfoLog(shader, maxLength, &maxLength, infoLog.data());

				// Clean up and log error
				glDeleteShader(shader);
				EE_CORE_ERROR("Shader compilation failed ({0}): {1}", shaderType, infoLog.data());

				for (GLuint id : shaderIDs) {
					glDeleteShader(id);
				}

				EE_CORE_ASSERT(false, "Shader compilation error");
				return;
			}

			// Attach and store shader ID
			glAttachShader(program, shader);
			shaderIDs[glShaderIDIndex] = shader;
			glShaderIDIndex++;
		}

		// Link program
		glLinkProgram(program);

		// Check linking status
		GLint isLinked = 0;
		glGetProgramiv(program, GL_LINK_STATUS, &isLinked);
		if (isLinked == GL_FALSE) {
			// Get and log linking error
			GLint maxLength = 0;
			glGetProgramiv(program, GL_INFO_LOG_LENGTH, &maxLength);
			std::vector<GLchar> infoLog(maxLength);
			glGetProgramInfoLog(program, maxLength, &maxLength, infoLog.data());

			// Clean up shaders and program
			glDeleteProgram(program);
			for (GLuint id : shaderIDs) {
				glDeleteShader(id);
			}

			EE_CORE_ERROR("Program linking failed: {0}", infoLog.data());
			EE_CORE_ASSERT(false, "Program linking error");
			return;
		}

		// Detach shaders after successful linking
		for (GLuint id : shaderIDs) {
			glDetachShader(program, id);
			glDeleteShader(id); // Free shader memory after detaching
		}

		m_renderID = program; // Assign the compiled and linked program
	}

	std::unordered_map<GLenum, std::string> OpenGLShader::PreProcess(const std::string& source) {
		EE_CORE_ASSERT(!source.empty(), "Shader source code is empty");

		std::unordered_map<GLenum, std::string> shaderSources;
		const std::string typeToken = "#type";
		size_t typeTokenLength = typeToken.length();
		size_t pos = source.find(typeToken); // Start of shader type declaration

		while (pos != std::string::npos) {
			// Find shader type declaration line
			size_t eol = source.find_first_of("\r\n", pos);
			EE_CORE_ASSERT(eol != std::string::npos, "Syntax error: Missing EOL after shader type declaration");

			// Extract shader type
			size_t begin = pos + typeTokenLength + 1;
			std::string type = source.substr(begin, eol - begin);
			GLenum shaderType = ShaderTypeFromString(type);
			EE_CORE_ASSERT(shaderType != 0, "Invalid shader type specified");

			// Extract shader code
			size_t nextLinePos = source.find_first_not_of("\r\n", eol);
			EE_CORE_ASSERT(nextLinePos != std::string::npos, "Syntax error: Missing shader code after type declaration");

			pos = source.find(typeToken, nextLinePos); // Start of next shader type
			shaderSources[shaderType] = (pos == std::string::npos)
				? source.substr(nextLinePos) // Take the rest of the source
				: source.substr(nextLinePos, pos - nextLinePos);
		}

		return shaderSources;
	}


	

	std::string OpenGLShader::ReadFile(const std::string& filepath)
	{
		EE_PROFILE_FUNCTION();

		std::ifstream in(filepath, std::ios::in | std::ios::binary);
		if (!in) {
			EE_CORE_ERROR("Could not open file: {0}", filepath);
			return ""; // Return an empty string on error
		}

		// Read the file contents
		std::string result;
		in.seekg(0, std::ios::end);
		result.resize(static_cast<size_t>(in.tellg()));
		in.seekg(0, std::ios::beg);
		in.read(result.data(), result.size());

		// Check for read failure
		if (in.fail()) {
			EE_CORE_ERROR("Error reading file: {0}", filepath);
			return "";
		}

		in.close();
		return result;
	}


}


