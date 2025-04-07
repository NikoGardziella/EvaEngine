#pragma once

#include "Engine/Renderer/Buffer.h"

namespace Engine {

	class OpenGLVertexBuffer : public VertexBuffer
	{
	public:

		OpenGLVertexBuffer(float* vertices, uint32_t size);
		OpenGLVertexBuffer(uint32_t size);

		virtual ~OpenGLVertexBuffer();

		virtual void Bind() const override;
		virtual void UnBind() const override;

		virtual void SetData(const void* data, uint32_t size) override;
		virtual void SetMat4InstanceAttribute(uint32_t location) override;

		virtual void SetLayout(const BufferLayout& layout) override { m_layout = layout;  }
		virtual const BufferLayout GetLayout()  const override { return m_layout; }
		virtual uint32_t GetSize() const override { return m_size; }

		void* GetBuffer() const override { EE_CORE_WARN("dont to this"); return nullptr; }

	private:

		uint32_t m_rendererID;
		uint32_t m_size;
		BufferLayout m_layout;

	};


	//********** OpenGLIndexBuffer ***************


	class OpenGLIndexBuffer : public IndexBuffer
	{
	public:

		OpenGLIndexBuffer(uint32_t* indices, uint32_t count);

		virtual ~OpenGLIndexBuffer();

		virtual void Bind() const override;
		virtual void UnBind() const override;
		 
		virtual uint32_t GetCount() const { return m_count; }
		virtual const void* GetData() const override { EE_CORE_WARN("dont to this"); return nullptr; }

		void* GetBuffer() const override { EE_CORE_WARN("dont to this");  return nullptr; }


	private:

		uint32_t m_rendererID;
		uint32_t m_count;

	};

}