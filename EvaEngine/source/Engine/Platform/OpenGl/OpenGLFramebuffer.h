#pragma once
#include "Engine/Renderer/Framebuffer.h"

namespace Engine {

	

	class OpenGLFramebuffer : public Framebuffer
	{

	public:
		OpenGLFramebuffer(const FramebufferSpecification& spec);
		virtual ~OpenGLFramebuffer();

		// Something has changed in framebuffer. recreate it
		void Invalidate();

		virtual void Bind() override;
		virtual void Unbind() override;
		virtual void Resize(uint32_t width, uint32_t height) override;

		virtual uint32_t GetColorAttachmentRendererID(uint32_t index = 0) const override { EE_CORE_ASSERT(index < m_colorAttachments.size()); return m_colorAttachments[index]; }

		virtual const FramebufferSpecification& GetSpecification() const override { return m_specification; }


	private:
		uint32_t m_rendererID = 0;
		FramebufferSpecification m_specification;


		std::vector<FramebufferTextureSpecification> m_colorAttachmentSpecs;
		FramebufferTextureSpecification m_depthAttachmentSpec;

		std::vector<uint32_t> m_colorAttachments;
		uint32_t m_depthAttachment = 0;
	};

}

