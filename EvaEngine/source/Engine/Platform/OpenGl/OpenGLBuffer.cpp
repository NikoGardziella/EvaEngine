#include "pch.h"
#include "OpenGLBuffer.h"

#include "glad/glad.h"

namespace Engine {

	
	OpenGLVertexBuffer::OpenGLVertexBuffer(float* vertices, uint32_t size)
	{
		EE_PROFILE_FUNCTION();


		glCreateBuffers(1,&m_rendererID);

		glBindBuffer(GL_ARRAY_BUFFER, m_rendererID);

		glBufferData(GL_ARRAY_BUFFER, size, vertices, GL_STATIC_DRAW);

	}

	OpenGLVertexBuffer::OpenGLVertexBuffer(uint32_t size)
	{
		EE_PROFILE_FUNCTION();

		glCreateBuffers(1, &m_rendererID);

		glBindBuffer(GL_ARRAY_BUFFER, m_rendererID);

		glBufferData(GL_ARRAY_BUFFER, size, nullptr, GL_DYNAMIC_DRAW); // nullptr - no data
	}

	OpenGLVertexBuffer::~OpenGLVertexBuffer()
	{
		glDeleteBuffers(1, &m_rendererID);

	}

	void OpenGLVertexBuffer::Bind() const
	{
		EE_PROFILE_FUNCTION();

		glBindBuffer(GL_ARRAY_BUFFER, m_rendererID);

	}

	void OpenGLVertexBuffer::UnBind() const
	{
		EE_PROFILE_FUNCTION();

		glBindBuffer(GL_ARRAY_BUFFER ,0);

	}

	

	void OpenGLVertexBuffer::SetData(const void* data, uint32_t size)
	{
		glBindBuffer(GL_ARRAY_BUFFER, m_rendererID);
		glBufferSubData(GL_ARRAY_BUFFER, 0, size, data);
	}

	//********** OpenGLIndexBuffer ***************

	OpenGLIndexBuffer::OpenGLIndexBuffer(uint32_t* indices, uint32_t count)
		: m_count(count)
	{
		EE_PROFILE_FUNCTION();

		glGenBuffers(1, &m_rendererID); // Use glGenBuffers for wider compatibility
		glBindBuffer(GL_ARRAY_BUFFER, m_rendererID);
		glBufferData(GL_ARRAY_BUFFER, count * sizeof(uint32_t), indices, GL_STATIC_DRAW);

		//glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0); // Unbind to prevent accidental modification
	}

	OpenGLIndexBuffer::~OpenGLIndexBuffer()
	{
		EE_PROFILE_FUNCTION();

		glDeleteBuffers(1, &m_rendererID);

	}

	void OpenGLIndexBuffer::Bind() const
	{
		EE_PROFILE_FUNCTION();

		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_rendererID);

	}

	void OpenGLIndexBuffer::UnBind() const
	{
		EE_PROFILE_FUNCTION();

		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

	}
}
