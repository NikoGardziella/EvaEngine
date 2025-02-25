#pragma once

#include "Engine/Renderer/Texture.h"
typedef unsigned int GLenum;


namespace Engine {

	class OpenGLTexture2D : public Texture2D
	{
	public:
		OpenGLTexture2D(const std::string& path);
		OpenGLTexture2D(uint32_t width, uint32_t height);

		virtual ~OpenGLTexture2D();

		virtual uint32_t GetWidth() const override { return m_width; }
		virtual uint32_t Getheight() const override { return m_height; }
		virtual void Bind(uint32_t slot = 0) const override;
		virtual uint32_t GetRendererID() const override { return m_rendererID; }

		virtual void SetData(void* data, uint32_t size) override;

		virtual bool operator==(const Texture& other) const override
		{
			// Try to cast 'other' to OpenGLTexture2D safely
			const OpenGLTexture2D* openglOther = dynamic_cast<const OpenGLTexture2D*>(&other);

			// If the cast fails, the objects are not of the same type, so return false
			if (!openglOther)
				return false;

			// Compare renderer IDs
			return m_rendererID == openglOther->m_rendererID;
		}

	private:
		std::string m_path;
		uint32_t m_width;
		uint32_t m_height;
		uint32_t m_rendererID;
		GLenum m_dataFormat;
		GLenum m_internalFormat;
	};

}

