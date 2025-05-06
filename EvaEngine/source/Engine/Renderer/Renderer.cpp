#include "pch.h"
#include "Renderer.h"

#include "OrthographicCamera.h"
#include <Engine/Platform/OpenGl/OpenGLShader.h>
#include "Engine/Renderer/Renderer2D.h"
#include "Engine/Renderer/VulkanRenderer2D.h"



namespace Engine {


	Renderer::SceneData* Renderer::m_sceneData = new SceneData();
	std::unique_ptr<Engine::VulkanRenderer2D> Engine::Renderer::s_VulkanRenderer2D = nullptr;

	uint32_t Renderer::s_currentFrame = 0;

	void Renderer::Init(RendererAPI::API api)
	{
		//std::unique_ptr<RendererAPI> rendererAPI = RendererAPI::CreateRenderer(selectedAPI);
		//rendererAPI->Init();
		
		RenderCommand::SetRendererAPI(api);

		RenderCommand::Init();
		if (api == RendererAPI::API::Vulkan)
		{
			s_VulkanRenderer2D = std::make_unique<VulkanRenderer2D>();
			s_VulkanRenderer2D->Init();
		}
		else
		{
			Renderer2D::Init();
		}
	}

	void Renderer::OnWindowResize(uint32_t width, uint32_t height)
	{
		RenderCommand::SetViewport(0, 0, width, height);
	}

	void Renderer::BeginScene(OrthographicCamera& camera)
	{
		m_sceneData->ViewProjectionMatrix = camera.GetViewProjectionMatrix();
	}

	void Renderer::DrawFrame()
	{
		EE_PROFILE_FUNCTION();

		if (s_VulkanRenderer2D)
		{
			//s_VulkanRenderer2D->BeginFrame(s_currentFrame);
			s_VulkanRenderer2D->DrawFrame(s_currentFrame);
			//s_VulkanRenderer2D->EndFrame(s_currentFrame);
		}
		else
		{
			Renderer2D::Flush();
			RenderCommand::Clear();
			RenderCommand::SetClearColor({ 0.1f, 0.1f, 0.1f, 1.0f });
		}

	}

	void Renderer::StartFrame()
	{
		EE_PROFILE_FUNCTION();
		s_VulkanRenderer2D->BeginFrame(s_currentFrame);

	}

	void Renderer::EndScene()
	{

	}

	void Renderer::EndFrame()
	{
		EE_PROFILE_FUNCTION();
		s_VulkanRenderer2D->EndFrame(s_currentFrame);

		Engine::VulkanRenderer2D::ResetStats();
		s_currentFrame = (s_currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
		
	}

	void Renderer::DeviceWaitIdle()
	{
		s_VulkanRenderer2D->DeviceWaitIdle();
	}

	void Renderer::Submit(const Ref<VertexArray>& vertexArray, const Ref<Shader>& shader, const glm::mat4& transform)
	{
		shader->Bind();
   // could be static cast?
		std::dynamic_pointer_cast<OpenGLShader>(shader)->UploadUniformMat4("u_viewProjection", m_sceneData->ViewProjectionMatrix);
		std::dynamic_pointer_cast<OpenGLShader>(shader)->UploadUniformMat4("u_transform", transform); // per object

		vertexArray->Bind();
		RenderCommand::DrawIndexed(vertexArray);
	}

	VkDescriptorSet Renderer::GetCurrentGameDescriptorSet()
	{
		return s_VulkanRenderer2D->GetGameDescriptorSet(s_currentFrame);
	}

}