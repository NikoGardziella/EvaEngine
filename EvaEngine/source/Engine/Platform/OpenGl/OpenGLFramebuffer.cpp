#include "pch.h"
#include "OpenGLFramebuffer.h"

#include "glad/glad.h"

namespace Engine {

	namespace Utils {

		static bool IsDepthFormat(FramebufferTextureFormat format)
		{
			switch (format)
			{

			case Engine::FramebufferTextureFormat::DEPTH24STENCIL8:
				return true;
				break;

			}
			return false;

		}

		static GLenum TextureTarget(bool multisampled)
		{
			return multisampled ? GL_TEXTURE_2D_MULTISAMPLE : GL_TEXTURE_2D;
		}

		static void CreateTextures(bool multisampled, uint32_t* outID, uint32_t count)
		{
			glCreateTextures(TextureTarget(multisampled), count, outID);
		}

		static void BindTexture(bool multisampled, uint32_t id)
		{
			glBindTexture(TextureTarget(multisampled), id);
		}

		static void AttachColorTexture(uint32_t id, int samples, GLenum internalFormat, GLenum format, uint32_t width, uint32_t height, int index)
		{
			bool multisampled = samples > 1;

			glBindTexture(TextureTarget(multisampled), id); // Ensure texture is bound

			if (multisampled)
			{
				glTexImage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, samples, internalFormat, width, height, GL_FALSE);
			}
			else
			{
				glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, width, height, 0, format, GL_UNSIGNED_BYTE, nullptr);

				// Apply filtering and wrapping only for non-multisample textures
				if (format == GL_RED_INTEGER)
				{
					glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
					glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
				}
				else
				{
					glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
					glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
				}

				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
			}

			glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + index, TextureTarget(multisampled), id, 0);
		}



		static void AttachDepthTexture(uint32_t id, int samples, GLenum format, GLenum attachmenType, uint32_t width, uint32_t height)
		{
			bool multisampled = samples > 1;
			if (multisampled)
			{
				glTexImage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, samples, format, width, height, GL_FALSE);
			}
			else
			{
				glTexStorage2D(GL_TEXTURE_2D, 1, format, width, height);

				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

			}
			// Attach the Texture to a Framebuffer
			glFramebufferTexture2D(GL_FRAMEBUFFER, attachmenType, TextureTarget(multisampled), id, 0);

		}


		static GLenum EngineTextureFormatToGL(FramebufferTextureFormat format)
		{
			switch (format)
			{
				case Engine::FramebufferTextureFormat::RGBA8:
					return GL_RGBA8;

				case Engine::FramebufferTextureFormat::RED_INTEGER:
					return GL_RED_INTEGER;
			}

			EE_CORE_ASSERT(false);
			return 0;
		}

		
	}


	OpenGLFramebuffer::OpenGLFramebuffer(const FramebufferSpecification& spec)
		: m_specification(spec)
	{
		for (auto spec : m_specification.Attachments.Attachments)
		{
			if (!Utils::IsDepthFormat(spec.TextureFormat))
			{
				m_colorAttachmentSpecs.emplace_back(spec);
			}
			else
			{
				m_depthAttachmentSpec = spec;
			}
		}

		Invalidate();
	}

	OpenGLFramebuffer::~OpenGLFramebuffer()
	{
		glDeleteFramebuffers(1, &m_rendererID);
		glDeleteTextures(m_colorAttachments.size(), m_colorAttachments.data());
		glDeleteTextures(1, &m_depthAttachment);

		m_colorAttachments.clear();
		m_depthAttachment = 0;

	}

	void OpenGLFramebuffer::Invalidate()
	{
		if (m_rendererID)
		{
			glDeleteFramebuffers(1, &m_rendererID);
			glDeleteTextures(m_colorAttachments.size(), m_colorAttachments.data());
			glDeleteTextures(1, &m_depthAttachment);
		}

		glCreateFramebuffers(1, &m_rendererID);
		glBindFramebuffer(GL_FRAMEBUFFER, m_rendererID);

		bool multiSample = m_specification.Samples > 1;

		if (m_colorAttachmentSpecs.size())
		{
			// These are always in sync
			m_colorAttachments.resize(m_colorAttachmentSpecs.size());
			Utils::CreateTextures(multiSample, m_colorAttachments.data(), m_colorAttachments.size());

			// Attachments
			for (size_t i = 0; i < m_colorAttachments.size(); i++)
			{
				Utils::BindTexture(multiSample, m_colorAttachments[i]);
				switch (m_colorAttachmentSpecs[i].TextureFormat)
				{
				case FramebufferTextureFormat::RGBA8:
					Utils::AttachColorTexture(m_colorAttachments[i], m_specification.Samples, GL_RGBA8, GL_RGBA, m_specification.Width, m_specification.Height, i);
					break;
				case FramebufferTextureFormat::RED_INTEGER:
					Utils::AttachColorTexture(m_colorAttachments[i], m_specification.Samples, GL_R32I, GL_RED_INTEGER, m_specification.Width, m_specification.Height, i);
					break;
				}
			}
		}

		if (m_depthAttachmentSpec.TextureFormat != FramebufferTextureFormat::None)
		{
			Utils::CreateTextures(multiSample, &m_depthAttachment, 1);
			Utils::BindTexture(multiSample, m_depthAttachment);
			switch (m_depthAttachmentSpec.TextureFormat)
			{
			case FramebufferTextureFormat::DEPTH24STENCIL8:
				Utils::AttachDepthTexture(m_depthAttachment, m_specification.Samples, GL_DEPTH24_STENCIL8, GL_DEPTH_ATTACHMENT, m_specification.Width, m_specification.Height);
				break;
			}
		}

		if (m_colorAttachments.size() > 1)
		{
			EE_CORE_ASSERT(m_colorAttachments.size() <= 4); // or a higher number if needed
			GLenum buffers[4] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2, GL_COLOR_ATTACHMENT3 };
			glDrawBuffers(m_colorAttachments.size(), buffers);
		}
		else if (m_colorAttachments.empty())
		{
			glDrawBuffer(GL_NONE);
		}

		EE_CORE_ASSERT(glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE, "Framebuffer is incomplete!");
		EE_CORE_INFO("Framebuffer status {}", glCheckFramebufferStatus(GL_FRAMEBUFFER) );

		glBindFramebuffer(GL_FRAMEBUFFER, 0);
	}


	void OpenGLFramebuffer::Bind()
	{

		glBindFramebuffer(GL_FRAMEBUFFER, m_rendererID);
		glViewport(0, 0, m_specification.Width, m_specification.Height);
	}

	void OpenGLFramebuffer::Unbind()
	{
		glBindFramebuffer(GL_FRAMEBUFFER, 0);

	}

	void OpenGLFramebuffer::Resize(uint32_t width, uint32_t height)
	{
		m_specification.Width = width;
		m_specification.Height = height;
		Invalidate();
		//glViewport(0, 0, width, height);
	}

	int OpenGLFramebuffer::ReadPixel(uint32_t attachmentIndex, int x, int y)
	{
		//glBindFramebuffer(GL_FRAMEBUFFER, m_rendererID); // Bind the framebuffer

		EE_CORE_ASSERT(attachmentIndex < m_colorAttachments.size());
		glReadBuffer(GL_COLOR_ATTACHMENT0 + attachmentIndex);
		int pixelData;
		glReadPixels(x, y, 1, 1, GL_RED_INTEGER, GL_INT, &pixelData);

		//glBindFramebuffer(GL_FRAMEBUFFER, 0); // Unbind framebuffer

		return pixelData;
	}

	void OpenGLFramebuffer::ClearColorAttachment(uint32_t attachmentIndex, int value)
	{
		EE_CORE_ASSERT(attachmentIndex < m_colorAttachments.size());

		auto& spec = m_colorAttachmentSpecs[attachmentIndex];

		// Assuming color attachments are typically in GL_FLOAT or GL_UNSIGNED_BYTE
		glClearTexImage(m_colorAttachments[attachmentIndex], 0,
			Utils::EngineTextureFormatToGL(spec.TextureFormat), GL_FLOAT, &value);
	}


	int OpenGLFramebuffer::GetFrameBufferStatus() const
	{
		GLint prevFramebuffer = 0;
		glGetIntegerv(GL_FRAMEBUFFER_BINDING, &prevFramebuffer); // Store currently bound framebuffer

		glBindFramebuffer(GL_FRAMEBUFFER, m_rendererID); // Bind our framebuffer
		int status = glCheckFramebufferStatus(GL_FRAMEBUFFER);

		glBindFramebuffer(GL_FRAMEBUFFER, prevFramebuffer); // Restore previous framebuffer

		return (int)status;

	}



}