#include "pch.h"
#include "VulkanRenderer2D.h"
#include "Engine/Platform/Vulkan/VulkanContext.h"
#include <Engine/Platform/Vulkan/VulkanGraphicsPipeline.h>
#include "Engine/AssetManager/AssetManager.h"

#include "VertexArray.h"
#include "Shader.h"
#include "Renderer2D.h"
#include "OrthographicCameraController.h"
#include "Renderer.h"
#include <backends/imgui_impl_vulkan.h>

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/string_cast.hpp>
#include <glm/gtx/matrix_decompose.hpp>

namespace Engine {

	VulkanRenderer2D::SceneData* VulkanRenderer2D::m_sceneData = new SceneData();

	
	struct VulkanRenderer2DData
	{
		static const uint32_t MaxQuads = 20000;
		static const uint32_t MaxVertices = MaxQuads * 4;
		static const uint32_t MaxIndices = MaxQuads * 6;
		static const uint32_t MaxTextureSlots = 32; // TODO: RenderCaps


	

		Ref<VertexArray> QuadVertexArray;
		Ref<VulkanVertexBuffer> QuadVertexBuffer;
		Ref<VulkanIndexBuffer> QuadIndexBuffer;
		Ref<VulkanShader> QuadShader;
		Ref<VulkanTexture> WhiteTexture;

		uint32_t QuadIndexCount = 0;
		VulkanQuadVertex* QuadVertexBufferBase = nullptr;
		VulkanQuadVertex* QuadVertexBufferPtr = nullptr;

		std::array<Ref<VulkanTexture>, MaxTextureSlots> TextureSlots;
		uint32_t TextureSlotIndex = 1; // 0 = white texture

		glm::vec4 QuadVertexPositions[4];

		Renderer2D::Statistics Stats;

		struct CameraData
		{
			glm::mat4 ViewProjection;
		};
		//CameraData CameraBuffer;
		//Ref<UniformBuffer> CameraUniformBuffer;
	};

	static VulkanRenderer2DData s_VulkanData;


	VulkanRenderer2D::VulkanRenderer2D()
	{
		

	}

	VulkanRenderer2D::~VulkanRenderer2D()
	{
		VkDevice device = m_vulkanContext->GetDeviceManager().GetDevice();
		for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
		{
			vkDestroySemaphore(device, m_imageAvailableSemaphores[i], nullptr);
			vkDestroySemaphore(device, m_renderFinishedSemaphores[i], nullptr);
			vkDestroyFence(device, m_inFlightFences[i], nullptr);
		}
	}

	void VulkanRenderer2D::Init()
	{
		s_VulkanData.QuadShader = std::make_shared<VulkanShader>(AssetManager::GetAssetPath("shaders/VulkanRenderer2D_Quad.GLSL").string());  // Add appropriate shader paths

		m_vulkanContext = VulkanContext::Get();
		m_swapchain = m_vulkanContext->GetVulkanSwapchain().GetSwapchain();
		m_swapchainExtent = m_vulkanContext->GetVulkanSwapchain().GetSwapchainExtent();
		m_vulkanGraphicsPipeline = std::make_shared<VulkanGraphicsPipeline>(m_vulkanContext->GetDeviceManager().GetDevice(),
			m_vulkanContext->GetVulkanSwapchain().GetSwapchainExtent(), m_vulkanContext->GetRenderPass(), s_VulkanData.QuadShader);
		m_device = m_vulkanContext->GetDeviceManager().GetDevice();

		// Allocate command buffers and sync objects
		AllocateCommandBuffers(m_vulkanContext->GetDeviceManager().GetDevice(), m_vulkanContext->GetCommandPool());
		CreateSyncObjects();

		// Create vertex buffer - convert to float* and size (in bytes)
		/*
		m_vertexBuffer = std::make_shared<VertexBuffer>(
			(float*)(quadVertices.data()), 
			sizeof(QuadVertex) * quadVertices.size()              
		);
		*/
		//m_vertexBuffer = std::make_shared<VulkanVertexBuffer>((float*)quadVertices.data(), sizeof(VulkanQuadVertex) * quadVertices.size());

		//EE_CORE_INFO("Vertex buffer created with size: {}", sizeof(VulkanQuadVertex) * quadVertices.size());

		
		//m_indexBuffer = std::make_shared<VulkanIndexBuffer>(quadIndices.data(), sizeof(VulkanQuadVertex) * quadVertices.size());
		//EE_CORE_INFO("Index buffer created with size: {}", sizeof(uint32_t) * quadVertices.size());

		m_camera = std::make_shared<OrthographicCamera>(-5.0f, 5.0f, -5.0f, 5.0f);
		m_camera->SetPosition({ 0.0f, 0.0f, 1.0f }); // Move the camera back to see the quad

		// Update the uniform buffer with the camera's view-projection matrix
		m_vulkanGraphicsPipeline->UpdateUniformBuffer(m_camera->GetViewProjectionMatrix());

		s_VulkanData.QuadShader = std::make_shared<VulkanShader>(AssetManager::GetAssetPath("shaders/VulkanRenderer2D_Quad.GLSL").string());  // Add appropriate shader paths
		//s_VulkanData.QuadVertexArray = VertexArray::Create();


		s_VulkanData.QuadVertexBufferBase = new VulkanQuadVertex[VulkanRenderer2DData::MaxVertices];
		s_VulkanData.QuadVertexBufferPtr = s_VulkanData.QuadVertexBufferBase;

		// Create VulkanVertexBuffer with nullptr for now
		s_VulkanData.QuadVertexBuffer = std::make_shared<VulkanVertexBuffer>(
			reinterpret_cast<float*>(s_VulkanData.QuadVertexBufferBase),
			VulkanRenderer2DData::MaxVertices * sizeof(VulkanQuadVertex)
		);

		std::vector<uint32_t> indices = { 0, 1, 2, 2, 3, 0 }; // Two triangles forming a quad
		s_VulkanData.QuadIndexBuffer = std::make_shared<VulkanIndexBuffer>(quadIndices.data(), quadIndices.size());
		// Can we remove this in vulkan?
		s_VulkanData.WhiteTexture = std::make_shared<VulkanTexture>(AssetManager::GetAssetPath("textures/white_texture.png").string());

		uint32_t whiteTextureData = 0xffffffff;
		s_VulkanData.WhiteTexture->SetData(&whiteTextureData, sizeof(uint32_t));

		int32_t samplers[s_VulkanData.MaxTextureSlots];
		for (uint32_t i = 0; i < s_VulkanData.MaxTextureSlots; i++)
		{
			samplers[i] = i;

		}
		s_VulkanData.TextureSlots[0] = s_VulkanData.WhiteTexture;
		s_VulkanData.TextureSlots[1] = AssetManager::AddTexture("logo", Engine::AssetManager::GetAssetPath("textures/ee_logo.png").string());
		s_VulkanData.TextureSlotIndex++;
		s_VulkanData.TextureSlots[2] = AssetManager::AddTexture("chess", Engine::AssetManager::GetAssetPath("textures/chess_board.png").string());
		s_VulkanData.TextureSlotIndex++;

		for (uint32_t i = 0; i < s_VulkanData.TextureSlotIndex; i++)
		{
			m_vulkanGraphicsPipeline->UpdateDescriptorSets(i, s_VulkanData.TextureSlots[i]);

		}
		
	}



	void VulkanRenderer2D::DrawFrame(uint32_t currentFrame)
	{
		
		m_vulkanGraphicsPipeline->UpdateUniformBuffer(m_sceneData->ViewProjectionMatrix);


		vkWaitForFences(m_device, 1, &m_inFlightFences[currentFrame], VK_TRUE, UINT64_MAX);
		vkResetFences(m_device, 1, &m_inFlightFences[currentFrame]);
		
		//

		VkResult result = vkAcquireNextImageKHR(
			m_device, m_swapchain, UINT64_MAX,
			m_imageAvailableSemaphores[currentFrame],
			VK_NULL_HANDLE, &currentFrame
		);

		if (result == VK_ERROR_OUT_OF_DATE_KHR)
		{
			m_vulkanContext->GetVulkanSwapchain().RecreateSwapchain();
			return;
		}
		else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR)
		{
			EE_CORE_ASSERT(false, "Failed to acquire swapchain image!");
		}

		vkResetCommandBuffer(m_commandBuffers[currentFrame], 0);


		RecordCommandBuffer(m_commandBuffers[currentFrame], currentFrame);

		VkSubmitInfo submitInfo = {};
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

		VkSemaphore waitSemaphores[] = { m_imageAvailableSemaphores[currentFrame] };
		VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
		submitInfo.waitSemaphoreCount = 1;
		submitInfo.pWaitSemaphores = waitSemaphores;
		submitInfo.pWaitDstStageMask = waitStages;

		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &m_commandBuffers[currentFrame];

		VkSemaphore signalSemaphores[] = { m_renderFinishedSemaphores[currentFrame] };
		submitInfo.signalSemaphoreCount = 1;
		submitInfo.pSignalSemaphores = signalSemaphores;

		if (vkQueueSubmit(m_vulkanContext->GetGraphicsQueue(), 1, &submitInfo, m_inFlightFences[currentFrame]) != VK_SUCCESS)
		{
			EE_CORE_ASSERT(false, "Failed to submit draw command buffer!");
			return;
		}

		VkPresentInfoKHR presentInfo = {};
		presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
		presentInfo.waitSemaphoreCount = 1;
		presentInfo.pWaitSemaphores = signalSemaphores;
		VkSwapchainKHR swapChains[] = { m_swapchain };
		presentInfo.swapchainCount = 1;
		presentInfo.pSwapchains = swapChains;
		presentInfo.pImageIndices = &currentFrame;

		VkResult presentResult = vkQueuePresentKHR(m_vulkanContext->GetGraphicsQueue(), &presentInfo);
		if (presentResult == VK_ERROR_OUT_OF_DATE_KHR || presentResult == VK_SUBOPTIMAL_KHR)
		{
			m_vulkanContext->GetVulkanSwapchain().RecreateSwapchain();
		}
		else if (presentResult != VK_SUCCESS)
		{
			EE_CORE_ASSERT(false, "Failed to present swapchain image!");
		}
	}








	void VulkanRenderer2D::AllocateCommandBuffers(VkDevice device, VkCommandPool commandPool)
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

	void VulkanRenderer2D::RecordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex)
	{
		VkCommandBufferBeginInfo beginInfo{};
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

		if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS)
		{
			EE_CORE_ERROR("Failed to begin recording command buffer!");
			return;
		}

		// === Begin your main render pass ===
		VkRenderPassBeginInfo renderPassInfo{};
		renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		renderPassInfo.renderPass = m_vulkanContext->GetRenderPass(); // your main render pass
		renderPassInfo.framebuffer = m_vulkanContext->GetSwapchainFramebuffer(imageIndex);
		renderPassInfo.renderArea.offset = { 0, 0 };
		renderPassInfo.renderArea.extent = m_vulkanContext->GetVulkanSwapchain().GetSwapchainExtent();

		VkClearValue clearColor = { {{0.2f, 0.2f, 0.2f, 1.0f}} };
		renderPassInfo.clearValueCount = 1;
		renderPassInfo.pClearValues = &clearColor;

		vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

		// === Your pipeline rendering ===
		vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_vulkanGraphicsPipeline->GetPipeline());

		VkViewport viewport = {
			0.0f, 0.0f,
			static_cast<float>(m_swapchainExtent.width),
			static_cast<float>(m_swapchainExtent.height),
			0.0f, 1.0f
		};
		vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

		VkRect2D scissor = { {0, 0}, m_swapchainExtent };
		vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

		VkBuffer vertexBuffers[] = { s_VulkanData.QuadVertexBuffer->GetBuffer() };
		VkDeviceSize offsets[] = { 0 };
		vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);
		vkCmdBindIndexBuffer(commandBuffer, s_VulkanData.QuadIndexBuffer->GetBuffer(), 0, VK_INDEX_TYPE_UINT32);

		VkDescriptorSet descriptorSet = m_vulkanGraphicsPipeline->GetDescriptorSet(imageIndex);
		
	 
		vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_vulkanGraphicsPipeline->GetPipelineLayout(),
			0, 1, &descriptorSet, 0, nullptr);

		vkCmdDrawIndexed(commandBuffer, s_VulkanData.QuadIndexCount, 1, 0, 0, 0);

		// === End your render pass ===
		vkCmdEndRenderPass(commandBuffer);

		
		// === Begin ImGui render pass ===
		VkRenderPassBeginInfo imguiRenderPassInfo{};
		imguiRenderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		imguiRenderPassInfo.renderPass = m_vulkanContext->GetImGuiRenderPass(); // ImGui render pass
		imguiRenderPassInfo.framebuffer = m_vulkanContext->GetImGuiFramebuffer(imageIndex); // same framebuffer
		imguiRenderPassInfo.renderArea.offset = { 0, 0 };
		imguiRenderPassInfo.renderArea.extent = m_vulkanContext->GetVulkanSwapchain().GetSwapchainExtent();

		VkClearValue imguiClearValue{};
		imguiClearValue.color = { {0.0f, 0.0f, 0.0f, 0.0f} }; // usually no clear needed
		imguiRenderPassInfo.clearValueCount = 0;
		imguiRenderPassInfo.pClearValues = nullptr;

		vkCmdBeginRenderPass(commandBuffer, &imguiRenderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

		// === ImGui rendering ===
		//ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), commandBuffer);

		// === End ImGui render pass ===
		vkCmdEndRenderPass(commandBuffer);

		// === Finish recording ===
		if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS)
		{
			EE_CORE_ASSERT(false, "Failed to record command buffer!");
		}
	}





	void VulkanRenderer2D::CreateSyncObjects()
	{
		VkSemaphoreCreateInfo semaphoreInfo{};
		semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

		// Initialize semaphores and fences
		m_imageAvailableSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
		m_renderFinishedSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
		m_inFlightFences.resize(MAX_FRAMES_IN_FLIGHT);

		VkFenceCreateInfo fenceInfo{};
		fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
		fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

		for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
		{
			if (vkCreateSemaphore(m_vulkanContext->GetDeviceManager().GetDevice(), &semaphoreInfo, nullptr, &m_imageAvailableSemaphores[i]) != VK_SUCCESS ||
				vkCreateSemaphore(m_vulkanContext->GetDeviceManager().GetDevice(), &semaphoreInfo, nullptr, &m_renderFinishedSemaphores[i]) != VK_SUCCESS ||
				vkCreateFence(m_vulkanContext->GetDeviceManager().GetDevice(), &fenceInfo, nullptr, &m_inFlightFences[i]) != VK_SUCCESS)
			{
				EE_CORE_ASSERT(false, "Failed to create synchronization objects for a frame!");

			}
		}
	}


	void VulkanRenderer2D::DrawQuad(const glm::mat4& transform, const std::shared_ptr<VulkanTexture>& texture, float tilingFactor, const glm::vec4& tintColor)
	{
		// Find texture slot index
		float textureIndex = 1.0f;
		
		for (uint32_t i = 1; i < s_VulkanData.TextureSlotIndex; i++)
		{
			if (*s_VulkanData.TextureSlots[i].get() == *texture.get())
			{
				textureIndex = (float)i;
				break;
			}
		}

		if (textureIndex == 0.0f)
		{
			if (s_VulkanData.TextureSlotIndex >= VulkanRenderer2DData::MaxTextureSlots)
			{
				// Handle texture slot overflow (e.g., flush and reset batch)
				EE_CORE_WARN("Texture slot limit reached!");
				return;
			}

			textureIndex = (float)s_VulkanData.TextureSlotIndex;
			s_VulkanData.TextureSlots[s_VulkanData.TextureSlotIndex] = texture;
			s_VulkanData.TextureSlotIndex++;
		}
		

		// Create transformed quad vertices
		const glm::vec3 quadPositions[4] = {
			{-0.5f, -0.5f, 0.0f},
			{ 0.5f, -0.5f, 0.0f},
			{ 0.5f,  0.5f, 0.0f},
			{-0.5f,  0.5f, 0.0f}
		};

		const glm::vec2 texCoords[4] = {
			{0.0f, 0.0f},
			{1.0f, 0.0f},
			{1.0f, 1.0f},
			{0.0f, 1.0f}
		};

		for (size_t i = 0; i < 4; i++)
		{
			glm::vec4 transformed = transform * glm::vec4(quadPositions[i], 1.0f);
			s_VulkanData.QuadVertexBufferPtr->Position = glm::vec3(transformed);
			s_VulkanData.QuadVertexBufferPtr->Color = tintColor;
			s_VulkanData.QuadVertexBufferPtr->TexCoord = texCoords[i];
			s_VulkanData.QuadVertexBufferPtr->TexIndex = textureIndex;
			s_VulkanData.QuadVertexBufferPtr->TilingFactor = tilingFactor;
			s_VulkanData.QuadVertexBufferPtr++;
		}

		s_VulkanData.QuadIndexCount += 6;
	}

	void VulkanRenderer2D::BeginScene(glm::mat4 viewProjectionMatrix)
	{

		
		m_sceneData->ViewProjectionMatrix = viewProjectionMatrix;


	}

	void VulkanRenderer2D::EndScene()
	{
		uint32_t dataSize = (uint32_t)((uint8_t*)s_VulkanData.QuadVertexBufferPtr - (uint8_t*)s_VulkanData.QuadVertexBufferBase);

		if (dataSize > 0)
		{
			s_VulkanData.QuadVertexBuffer->SetData(s_VulkanData.QuadVertexBufferBase, dataSize);
		}

		s_VulkanData.QuadVertexBufferPtr = s_VulkanData.QuadVertexBufferBase;
		s_VulkanData.QuadIndexCount = 0;
	}



	
	
}
