#pragma once
#include "Engine/Renderer/Buffer.h"

namespace Engine {

    class VulkanVertexBuffer : public VertexBuffer
    {
    public:
		VulkanVertexBuffer(float* vertices, uint32_t size);
		VulkanVertexBuffer(uint32_t size);

		virtual ~VulkanVertexBuffer();

		virtual void Bind() const override;
		virtual void UnBind() const override;

		virtual void SetData(const void* data, uint32_t size) override;
		virtual void SetMat4InstanceAttribute(uint32_t location) override;

		virtual void SetLayout(const BufferLayout& layout) override { m_layout = layout; }
		virtual const BufferLayout GetLayout()  const override { return m_layout; }
	private:

		uint32_t m_rendererID;
		BufferLayout m_layout;

    };

}

