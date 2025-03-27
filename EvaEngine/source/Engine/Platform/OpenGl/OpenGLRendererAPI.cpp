#include "pch.h"
#include "OpenGLRendererAPI.h"

#include "glad/glad.h"
#include "Engine/Core/Core.h"

namespace Engine {

	void OpenGLRendererAPI::Init()
	{
		EE_PROFILE_FUNCTION();


		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

		glEnable(GL_DEPTH_TEST);

	}

	void OpenGLRendererAPI::SetViewport(uint32_t x, uint32_t y, uint32_t width, uint32_t height)
	{
		glViewport(x, y, width, height);
	}

	void OpenGLRendererAPI::SetClearColor(const glm::vec4& color)
	{
		glClearColor(color.r, color.g, color.b, color.a);
	}

	

	void OpenGLRendererAPI::Clear()
	{
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	}

	void OpenGLRendererAPI::DrawIndexed(const Ref<VertexArray>& vertexArray, uint32_t indexCount)
	{
		EE_PROFILE_FUNCTION();

		vertexArray->Bind();
		//glDisable(GL_DEPTH_TEST);
		// Get the count from the IndexBuffer if indexCount is zero
		uint32_t count = indexCount;
		if (count == 0)
		{
			auto indexBuffer = vertexArray->GetIndexBuffer();
			if (indexBuffer)
			{
				count = indexBuffer->GetCount();
			}
		}

		if (count == 0)
		{
			return; // No indices to render, exit early
		}

		glDrawElements(GL_TRIANGLES, count, GL_UNSIGNED_INT, nullptr);
	}


	void OpenGLRendererAPI::DrawLines(const Ref<VertexArray>& vertexArray, uint32_t vertexCount)
	{
		// If indexCount is non-zero, use it. Otherwise, use the index count from the index buffer.
		if (vertexCount == 0)
		{
			return;
		}

		vertexArray->Bind();
		glDrawArrays(GL_LINES,0, vertexCount);

	}

	void OpenGLRendererAPI::DrawIndexedInstanced(const Ref<VertexArray>& vertexArray, uint32_t indexCount, uint32_t instanceCount)
	{
		
		vertexArray->Bind();


		// Get the count from the IndexBuffer if indexCount is zero
		uint32_t count = indexCount;
		if (count == 0)
		{
			auto indexBuffer = vertexArray->GetIndexBuffer();
			if (indexBuffer)
			{
				count = indexBuffer->GetCount();
			}
		}

		if (count == 0)
		{
			return; // No indices to render, exit early
		}

		// Use instanced draw to render multiple instances
		glDrawElementsInstanced(GL_TRIANGLES, count, GL_UNSIGNED_INT, nullptr, instanceCount);
		glDrawArraysInstanced(GL_TRIANGLES, 0, count, instanceCount);
	}




	void OpenGLRendererAPI::SetLineWidth(float thickness)
	{
		glLineWidth(thickness);
	}
}