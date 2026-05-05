#include "pch.h"
#include "Renderer.h"

#include "OrthographicCamera.h"
#include <Engine/Platform/OpenGl/OpenGLShader.h>
#include "Engine/Renderer/Renderer2D.h"
#include <Engine/Renderer/Renderer2D/VulkanRenderer2D.h>



namespace Engine {


	Renderer::SceneData* Renderer::m_sceneData = new SceneData();
	std::unique_ptr<Engine::VulkanRenderer2D> Engine::Renderer::s_VulkanRenderer2D = nullptr;
	std::unique_ptr<Engine::VulkanRenderer3D> Engine::Renderer::s_VulkanRenderer3D = nullptr;
	std::unique_ptr<Engine::VulkanSharedResources> Engine::Renderer::s_vulkanSharedResources = nullptr;
	std::vector<VkCommandBuffer> Renderer::m_commandBuffers;

	uint32_t Renderer::s_currentFrame = 0;

	void Renderer::Init(RendererAPI::API api)
	{
		//std::unique_ptr<RendererAPI> rendererAPI = RendererAPI::CreateRenderer(selectedAPI);
		//rendererAPI->Init();
		
		RenderCommand::SetRendererAPI(api);

		RenderCommand::Init();
		if (api == RendererAPI::API::Vulkan)
		{

			VulkanContext* vulkanContext = VulkanContext::Get();

			s_vulkanSharedResources = std::make_unique<VulkanSharedResources>();
			s_vulkanSharedResources->Init(vulkanContext);

			s_VulkanRenderer2D = std::make_unique<VulkanRenderer2D>();
			s_VulkanRenderer2D->Init(s_vulkanSharedResources->GetShadowMap());

			s_VulkanRenderer3D = std::make_unique<VulkanRenderer3D>();
			s_VulkanRenderer3D->InitVulkanRenderer3D(s_vulkanSharedResources->GetShadowMap());


			// MOOVE
			s_vulkanSharedResources->GetShadowMap()->GetShadowPipeline()->Create3DShadowPipeline(vulkanContext->GetDeviceManager().GetDevice(),
				s_vulkanSharedResources->GetShadowMap()->Get3DShadowmap().renderPass, s_VulkanRenderer3D->Get3DDescriptorSetLayout());

			s_vulkanSharedResources->GetShadowMap()->GetShadowPipeline()->CreateGroundShadowPipeline(vulkanContext->GetDeviceManager().GetDevice(),
				s_vulkanSharedResources->GetShadowMap()->GetTileShadowmap().renderPass);

			s_vulkanSharedResources->GetShadowMap()->GetShadowPipeline()->CreateTilesShadowPipeline(vulkanContext->GetDeviceManager().GetDevice(),
				s_vulkanSharedResources->GetShadowMap()->GetTileShadowmap().renderPass, s_VulkanRenderer2D->GetBindlessDescriptorSetRenderer()->GetSetLayout());




			AllocateCommandBuffers(vulkanContext->GetDeviceManager().GetDevice(), vulkanContext->GetCommandPool());


			


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
			Engine::VulkanRenderer3D::ResetStats3D(); // there is probably better place for this

			//s_VulkanRenderer2D->BeginFrame(s_currentFrame);

			//s_VulkanRenderer2D->RecordShadowPass(m_commandBuffers[s_currentFrame], s_currentFrame, s_vulkanSharedResources->GetShadowMap());
			// ground
			s_VulkanRenderer2D->DrawFrame(s_currentFrame, m_commandBuffers[s_currentFrame]);
			s_VulkanRenderer2D->DrawFogOverlay(m_commandBuffers[s_currentFrame]);
			//s_VulkanRenderer2D->RecordFogOfWarCommandBuffer(m_commandBuffers[s_currentFrame], s_currentFrame);
			

			uint32_t drawBehindPLayer = 0;
			s_VulkanRenderer2D->RecordUnderlayLineCommanedBuffer(m_commandBuffers[s_currentFrame], s_currentFrame);
			s_VulkanRenderer2D->DrawTiles(s_currentFrame, m_commandBuffers[s_currentFrame], s_vulkanSharedResources->GetShadowMap(), drawBehindPLayer);

			s_VulkanRenderer3D->Draw(s_currentFrame, m_commandBuffers[s_currentFrame]);

			// front of player
			uint32_t drawFrontPLayer = 1;
			s_VulkanRenderer2D->DrawTiles(s_currentFrame, m_commandBuffers[s_currentFrame], s_vulkanSharedResources->GetShadowMap(), drawFrontPLayer);


			
			//s_VulkanRenderer2D->EndFrame(s_currentFrame);
		}
		else
		{
			Renderer2D::Flush();
			RenderCommand::Clear();
			RenderCommand::SetClearColor({ 0.1f, 0.1f, 0.1f, 1.0f });
		}

	}

	void Renderer::DrawShadowFrame()
	{

		s_vulkanSharedResources->GetShadowMap()->TransitionToReadable(m_commandBuffers[s_currentFrame], s_vulkanSharedResources->GetShadowMap()->Get3DShadowmap());
		s_vulkanSharedResources->GetShadowMap()->TransitionToReadable(m_commandBuffers[s_currentFrame], s_vulkanSharedResources->GetShadowMap()->GetTileShadowmap());


		s_VulkanRenderer3D->DrawShadowPass(m_commandBuffers[s_currentFrame], s_currentFrame, s_vulkanSharedResources->GetShadowMap());

		s_VulkanRenderer2D->DrawTilesShadowPass(m_commandBuffers[s_currentFrame], s_currentFrame, s_vulkanSharedResources->GetShadowMap());

		//s_VulkanRenderer2D->RecordGameShadowPass(m_commandBuffers[s_currentFrame], s_currentFrame, s_vulkanSharedResources->GetShadowMap()->GetShadowPipeline()->GetGroundShadowPipeline(),
		//	s_vulkanSharedResources->GetShadowMap()->GetShadowPipeline()->GetGroundShadowPipelineLayout(), s_vulkanSharedResources->GetShadowMap()->GetLightSpaceMatrix());
		

		
	}

	void Renderer::StartFrame()
	{

		EE_PROFILE_FUNCTION();
		s_VulkanRenderer2D->BeginFrame(s_currentFrame);
		s_VulkanRenderer3D->BeginFrame3D(s_currentFrame);

	}

	void Renderer::EndScene()
	{

	}

	void Renderer::EndFrame()
	{
		EE_PROFILE_FUNCTION();
		s_VulkanRenderer2D->EndFrame(s_currentFrame, m_commandBuffers[s_currentFrame]);

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
		return s_VulkanRenderer2D->GetCurrentGameDescriptorSet();
	}


	void Renderer::AllocateCommandBuffers(VkDevice device, VkCommandPool commandPool)
	{
		m_commandBuffers.resize(MAX_FRAMES_IN_FLIGHT);

		VkCommandBufferAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		allocInfo.commandPool = commandPool;
		allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		allocInfo.commandBufferCount = static_cast<uint32_t>(m_commandBuffers.size());

		if (vkAllocateCommandBuffers(device, &allocInfo, m_commandBuffers.data()) != VK_SUCCESS)
		{
			EE_CORE_ERROR("Failed to allocate command buffers!");
		}

	}


}