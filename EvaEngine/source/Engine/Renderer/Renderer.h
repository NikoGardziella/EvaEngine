#pragma once

#include "RenderCommand.h"
#include "RendererAPI.h"
#include "Shader.h"
#include "OrthographicCamera.h"
#include "VulkanRenderer2D.h"

namespace Engine {

	const int MAX_FRAMES_IN_FLIGHT = 3;


	class Renderer
	{
		struct Statistics
		{
			uint32_t DrawCalls = 0;
			uint32_t QuadCount = 0;

			uint32_t GetTotalVertexCount() { return QuadCount * 4; }
			uint32_t GetTotalIndexCount() { return QuadCount * 6; }
		};
	public:

		static void Init(RendererAPI::API api);

		static void OnWindowResize(uint32_t width, uint32_t height);

		static void BeginScene(OrthographicCamera& camera);

		static void DrawFrame();

		static void EndScene();

		static void Submit(const Ref<VertexArray>& vertexArray, const Ref<Shader>& shader, const glm::mat4& transform = glm::mat4(1.0f));

		static uint32_t GetCurrentFrame() { return s_currentFrame; }


		inline static RendererAPI::API GetAPI() { return RendererAPI::GetAPI(); }
		static VulkanRenderer2D& Get2DRenderer() { return *s_VulkanRenderer2D; }
	private:
		struct SceneData
		{
			glm::mat4 ViewProjectionMatrix;
		};

		static SceneData* m_sceneData;
		static std::unique_ptr<VulkanRenderer2D> s_VulkanRenderer2D;
		static uint32_t s_currentFrame;
	};
}


