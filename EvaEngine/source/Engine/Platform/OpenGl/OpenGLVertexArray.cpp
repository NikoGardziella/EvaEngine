#include "pch.h"
#include "OpenGLVertexArray.h"
#include <glad/glad.h>
#include <Engine/Core/Core.h>


namespace Engine {

	static GLenum ShaderDataToOpenGLbaseType(ShaderDataType type)
	{
		switch (type)
		{

		case Engine::ShaderDataType::Float:
			return GL_FLOAT;
		case Engine::ShaderDataType::Float2:
			return GL_FLOAT;
		case Engine::ShaderDataType::Float3:
			return GL_FLOAT;
		case Engine::ShaderDataType::Float4:
			return GL_FLOAT;
		case Engine::ShaderDataType::Mat3:
			return GL_FLOAT;
		case Engine::ShaderDataType::Mat4:
			return GL_FLOAT;
		case Engine::ShaderDataType::Int:
			return GL_INT;
		case Engine::ShaderDataType::Int2:
			return GL_INT;
		case Engine::ShaderDataType::Int3:
			return GL_INT;
		case Engine::ShaderDataType::Int4:
			return GL_INT;
		case Engine::ShaderDataType::Bool:
			return GL_BOOL;
		}
		EE_CORE_ASSERT(false, "Unknown ShaderDataType!");
		return 0;
	}

	OpenGLVertexArray::OpenGLVertexArray()
	{
		EE_PROFILE_FUNCTION();

		glCreateVertexArrays(1, &m_rendererID);
	}

	OpenGLVertexArray::~OpenGLVertexArray()
	{
		glDeleteVertexArrays(1, &m_rendererID);
	}

	void OpenGLVertexArray::Bind() const
	{
		EE_PROFILE_FUNCTION();


		glBindVertexArray(m_rendererID);
	}

	void OpenGLVertexArray::UnBind() const
	{
		EE_PROFILE_FUNCTION();

		glBindVertexArray(0);

	}

    void OpenGLVertexArray::AddVertexBuffer(const Ref<VertexBuffer>& vertexBuffer)
    {
        EE_PROFILE_FUNCTION();

        glBindVertexArray(m_rendererID);
        vertexBuffer->Bind();

        EE_CORE_ASSERT(vertexBuffer->GetLayout().GetElements().size(), "VertexBuffer has no layout!");

        uint32_t index = 0;
        const auto& layout = vertexBuffer->GetLayout();

        for (const auto& element : layout)
        {
            glEnableVertexAttribArray(index);

            // Bind attributes based on ShaderDataType
            switch (element.Type)
            {
            case ShaderDataType::Float:
            case ShaderDataType::Float2:
            case ShaderDataType::Float3:
            case ShaderDataType::Float4:
                glVertexAttribPointer(
                    index,
                    element.GetComponentCount(),
                    ShaderDataToOpenGLbaseType(element.Type),
                    element.Normalized ? GL_TRUE : GL_FALSE,
                    layout.GetStride(),
                    (const void*)(intptr_t)element.Offset
                );
                break;

            case ShaderDataType::Int:
            case ShaderDataType::Int2:
            case ShaderDataType::Int3:
            case ShaderDataType::Int4:
                glVertexAttribIPointer( // Integer attributes need a different function
                    index,
                    element.GetComponentCount(),
                    ShaderDataToOpenGLbaseType(element.Type),
                    layout.GetStride(),
                    (const void*)(intptr_t)element.Offset
                );
                break;

            case ShaderDataType::Bool:
                glVertexAttribPointer(
                    index,
                    1,
                    GL_BOOL,
                    GL_FALSE,
                    layout.GetStride(),
                    (const void*)(intptr_t)element.Offset
                );
                break;

            case ShaderDataType::Mat3:
            case ShaderDataType::Mat4:
            {
                // Matrices are treated as multiple attribute locations
                uint8_t count = element.GetComponentCount();
                for (uint8_t i = 0; i < count; i++)
                {
                    glEnableVertexAttribArray(index + i);
                    glVertexAttribPointer(
                        index + i,
                        count, // Each column is a separate attribute
                        ShaderDataToOpenGLbaseType(element.Type),
                        element.Normalized ? GL_TRUE : GL_FALSE,
                        layout.GetStride(),
                        (const void*)(intptr_t)(element.Offset + sizeof(float) * count * i)
                    );
                    glVertexAttribDivisor(index + i, 1); // Needed for instanced rendering
                }
                index += count - 1; // Adjust index for the matrix
                break;
            }

            default:
                EE_CORE_ASSERT(false, "Unknown ShaderDataType!");
            }

            index++;
        }

        m_vertexBuffers.push_back(vertexBuffer);
    }


	void OpenGLVertexArray::SetIndexBuffer(const Ref<IndexBuffer>& indexBuffer)
	{
		glBindVertexArray(m_rendererID);
		indexBuffer->Bind();
		m_indexBuffer = indexBuffer;
	}

}