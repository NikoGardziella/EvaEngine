#pragma once
#include "Engine/Renderer/Framebuffer.h"

namespace Engine {

	class OpenGLFramebuffer : public Framebuffer
	{

	public:
		OpenGLFramebuffer(const FrameBufferSpecification& spec);
		virtual ~OpenGLFramebuffer();

		void Invalidate();

		virtual void Bind() override;
		virtual void Unbind() override;
		virtual void Resize(uint32_t width, uint32_t height) override;

		virtual uint32_t GetColorAttachmentRendererID() const override { return m_colorAttachement; }

		virtual const FrameBufferSpecification& GetSpecification() const override { return m_specification; }


	private:
		uint32_t m_rendererID = 0;
		FrameBufferSpecification m_specification;
		uint32_t m_colorAttachement = 0;
		uint32_t m_depthAttachment = 0;
	};

}

