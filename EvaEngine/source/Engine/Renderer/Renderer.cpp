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
				s_vulkanSharedResources->GetShadowMap()->GetShadowRenderPass(), s_VulkanRenderer3D->Get3DDescriptorSetLayout());

			s_vulkanSharedResources->GetShadowMap()->GetShadowPipeline()->CreateGroundShadowPipeline(vulkanContext->GetDeviceManager().GetDevice(),
				s_vulkanSharedResources->GetShadowMap()->GetShadowRenderPass());

			s_vulkanSharedResources->GetShadowMap()->GetShadowPipeline()->CreateTilesShadowPipeline(vulkanContext->GetDeviceManager().GetDevice(),
				s_vulkanSharedResources->GetShadowMap()->GetShadowRenderPass(), s_VulkanRenderer2D->GetBindlessDescriptorSetRenderer()->GetSetLayout());




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


			s_VulkanRenderer2D->DrawFrame(s_currentFrame, m_commandBuffers[s_currentFrame]);

			s_VulkanRenderer3D->Draw(s_currentFrame, m_commandBuffers[s_currentFrame]);

			s_VulkanRenderer2D->DrawTiles(s_currentFrame, m_commandBuffers[s_currentFrame]);

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
		s_VulkanRenderer3D->DrawShadowPass(m_commandBuffers[s_currentFrame], s_currentFrame, s_vulkanSharedResources->GetShadowMap());

		s_VulkanRenderer2D->DrawTilesShadowPass(m_commandBuffers[s_currentFrame], s_currentFrame, s_vulkanSharedResources->GetShadowMap());

		//s_VulkanRenderer2D->RecordGameShadowPass(m_commandBuffers[s_currentFrame], s_currentFrame, s_vulkanSharedResources->GetShadowMap()->GetShadowPipeline()->GetGroundShadowPipeline(),
		//	s_vulkanSharedResources->GetShadowMap()->GetShadowPipeline()->GetGroundShadowPipelineLayout(), s_vulkanSharedResources->GetShadowMap()->GetLightSpaceMatrix());
		
		vkCmdEndRenderPass(m_commandBuffers[s_currentFrame]);

		VkImageMemoryBarrier barrier{};
		barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		barrier.srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
		barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
		barrier.oldLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL; // because finalLayout
		barrier.newLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
		barrier.image = s_vulkanSharedResources->GetShadowMap()->GetShadowMapImage();              // <- your VkImage
		barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
		barrier.subresourceRange.baseMipLevel = 0;
		barrier.subresourceRange.levelCount = 1;
		barrier.subresourceRange.baseArrayLayer = 0;
		barrier.subresourceRange.layerCount = 1;

		vkCmdPipelineBarrier(
			m_commandBuffers[s_currentFrame],
			VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT,
			VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
			0,
			0, nullptr,
			0, nullptr,
			1, &barrier
		);
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
		return s_VulkanRenderer2D->GetGameDescriptorSet(s_currentFrame);
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