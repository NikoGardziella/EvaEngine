#pragma once

#include "Engine/Core/Core.h"

namespace Engine {

	enum class FramebufferTextureFormat
	{
		None = 0,

		// color
		RGBA8,
		RED_INTEGER,


		//depth, stencil
		DEPTH24STENCIL8,

		// Defaults
		Depth = DEPTH24STENCIL8,



	};

	struct FramebufferTextureSpecification
	{
		FramebufferTextureSpecification()
			: TextureFormat(FramebufferTextureFormat::None) {
		}

		FramebufferTextureSpecification(FramebufferTextureFormat format)
			: TextureFormat(format) {
		}

		FramebufferTextureFormat TextureFormat = FramebufferTextureFormat::None;
	};


	struct FramebufferAttachmentSpecification
	{
		FramebufferAttachmentSpecification() = default;
		FramebufferAttachmentSpecification(const std::initializer_list<FramebufferTextureSpecification> attachments)
			: Attachments(attachments) {
		}
		std::vector<FramebufferTextureSpecification> Attachments;
	};

	struct FramebufferSpecification
	{
		uint32_t Width = 0;
		uint32_t Height = 0;
		uint32_t Samples = 1;
		FramebufferAttachmentSpecification Attachments;


		bool SpawChainTarget = false;
	};

	class Framebuffer
	{

	public:

		virtual ~Framebuffer() = default;
		virtual void Bind() = 0;
		virtual void Unbind() = 0;

		virtual void Resize(uint32_t width, uint32_t height) = 0;
		virtual int ReadPixel(uint32_t attachmentIndex, int x, int y) = 0;

		virtual void ClearColorAttachment(uint32_t attachmentIndex, int value) = 0;


		virtual uint32_t GetColorAttachmentRendererID(uint32_t index = 0) const = 0;
		virtual const FramebufferSpecification& GetSpecification() const = 0;
		//virtual FrameBufferSpecification& GetSpecification() = 0;

		static Ref<Framebuffer> Create(const FramebufferSpecification& spec);

	private:


	};

}

