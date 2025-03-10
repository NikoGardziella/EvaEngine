#include "pch.h"
#include "OpenGLTexture.h"

#include "stb_image.h"
#include <Engine/Core/Core.h>
#include <glad/glad.h>

namespace Engine {

	OpenGLTexture2D::OpenGLTexture2D(const std::string& path)
		: m_path(path)
	{
		EE_PROFILE_FUNCTION();


		int width;
		int height;
		int channels;
		stbi_uc* data = nullptr;
		stbi_set_flip_vertically_on_load(1);
		{
			EE_PROFILE_SCOPE("OpenGLTexture2D::OpenGLTexture2D(const std::string& path) - stbi_load");
			data =  stbi_load(path.c_str(), &width, &height, &channels, 0);
		}

		EE_CORE_ASSERT(data, "failed to load image");

		m_width = width;
		m_height = height;

		GLenum interalFormat = 0;
		GLenum dataFormat = 0;
		if (channels == 4)
		{
			interalFormat = GL_RGBA8;
			dataFormat = GL_RGBA;
		}
		else if (channels == 3)
		{
			interalFormat = GL_RGB8;
			dataFormat = GL_RGB;
		}

		m_internalFormat = interalFormat;
		m_dataFormat = dataFormat;

		glCreateTextures(GL_TEXTURE_2D, 1, &m_rendererID);

		// Allocate immutable storage for the 2D texture.
		// Parameters: texture ID, 1 mipmap level, internal format, width, and height.
		glTextureStorage2D(m_rendererID, 1, interalFormat, m_width, m_height);

		// Set texture filtering parameters:

		// Set the minification filter to linear interpolation (used when the texture is scaled down).
		glTextureParameteri(m_rendererID, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

		// Set the magnification filter to nearest neighbor (used when the texture is scaled up).
		glTextureParameteri(m_rendererID, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

		// Set the texture wrapping mode on the S coordinate (horizontal axis) to repeat,
		// so the texture will tile if texture coordinates fall outside the range [0,1].
		glTextureParameteri(m_rendererID, GL_TEXTURE_WRAP_S, GL_REPEAT);

		glTextureParameteri(m_rendererID, GL_TEXTURE_WRAP_T, GL_REPEAT);




		// Upload the texture image data to the GPU.
		// This call replaces the content of the texture at mipmap level 0,
		// starting at offset (0,0) and covering the entire width and height.
		// The image data is provided in the specified data format with each element as an unsigned byte.
		glTextureSubImage2D(m_rendererID, 0, 0, 0, m_width, m_height, dataFormat, GL_UNSIGNED_BYTE, data);
		stbi_image_free(data);

	}

	OpenGLTexture2D::OpenGLTexture2D(uint32_t width, uint32_t height)
		: m_width(width), m_height(height)
	{
		EE_PROFILE_FUNCTION();

		m_internalFormat = GL_RGBA8;
		m_dataFormat = GL_RGBA;
		
		glCreateTextures(GL_TEXTURE_2D, 1, &m_rendererID);

		// Allocate immutable storage for the 2D texture.
		// Parameters: texture ID, 1 mipmap level, internal format, width, and height.
		glTextureStorage2D(m_rendererID, 1, m_internalFormat, m_width, m_height);

		// Set texture filtering parameters:

		// Set the minification filter to linear interpolation (used when the texture is scaled down).
		glTextureParameteri(m_rendererID, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

		// Set the magnification filter to nearest neighbor (used when the texture is scaled up).
		glTextureParameteri(m_rendererID, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

		// Set the texture wrapping mode on the S coordinate (horizontal axis) to repeat,
		// so the texture will tile if texture coordinates fall outside the range [0,1].
		glTextureParameteri(m_rendererID, GL_TEXTURE_WRAP_S, GL_REPEAT);

		glTextureParameteri(m_rendererID, GL_TEXTURE_WRAP_T, GL_REPEAT);



	}

	OpenGLTexture2D::~OpenGLTexture2D()
	{
		EE_PROFILE_FUNCTION();

		glDeleteTextures(1, &m_rendererID);
	}

	void OpenGLTexture2D::Bind(uint32_t slot) const
	{
		EE_PROFILE_FUNCTION();

		glBindTextureUnit(slot, m_rendererID);
	}
	void OpenGLTexture2D::SetData(void* data, uint32_t size)
	{
		EE_PROFILE_FUNCTION();

		uint32_t bytesPerChannel = m_dataFormat == GL_RGBA ? 4 : 3;
		EE_CORE_ASSERT(size == m_width * m_height * bytesPerChannel, " Data must be entire texture");
		glTextureSubImage2D(m_rendererID, 0, 0, 0, m_width, m_height, m_dataFormat, GL_UNSIGNED_BYTE, data);

	}
}