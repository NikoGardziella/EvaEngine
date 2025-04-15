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

		glm::vec3 QuadVertexPositions[4];


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

		s_VulkanData.QuadVertexPositions[0] = { -0.5f, -0.5f, 0.0f };
		s_VulkanData.QuadVertexPositions[1] = {  0.5f, -0.5f, 0.0f };
		s_VulkanData.QuadVertexPositions[2] = {  0.5f,  0.5f, 0.0f };
		s_VulkanData.QuadVertexPositions[3] = { -0.5f,  0.5f, 0.0f };

		std::vector<uint32_t> indices;
		indices.reserve(VulkanRenderer2DData::MaxIndices);

		uint32_t offset = 0;
		for (uint32_t i = 0; i < VulkanRenderer2DData::MaxQuads; i++)
		{
			indices.push_back(offset + 0);
			indices.push_back(offset + 1);
			indices.push_back(offset + 2);
			indices.push_back(offset + 2);
			indices.push_back(offset + 3);
			indices.push_back(offset + 0);
			offset += 4;
		}
		
		s_VulkanData.QuadIndexBuffer = std::make_shared<VulkanIndexBuffer>(indices.data(), indices.size());		// Can we remove this in vulkan?
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
		CreateImGuiTextureDescriptors();
		m_imageLayouts.resize(m_vulkanContext->GetVulkanSwapchain().GetSwapchainImages().size(), VK_IMAGE_LAYOUT_UNDEFINED);
		m_gameColorLayouts.resize(m_vulkanContext->GetVulkanSwapchain().GetSwapchainImages().size(), VK_IMAGE_LAYOUT_UNDEFINED);
	}



	void VulkanRenderer2D::DrawFrame(uint32_t currentFrame)
	{
		


		m_vulkanGraphicsPipeline->UpdateUniformBuffer(m_sceneData->ViewProjectionMatrix);

		VulkanContext* vulkanContext = VulkanContext::Get();

		// 1. Wait for the current frame to finish
		vkWaitForFences(m_device, 1, &m_inFlightFences[currentFrame], VK_TRUE, UINT64_MAX);
		vkResetFences(m_device, 1, &m_inFlightFences[currentFrame]);

		// 2. Acquire image FIRST!
		uint32_t imageIndex;
		VkResult result = vkAcquireNextImageKHR(
			m_device, m_swapchain, UINT64_MAX,
			m_imageAvailableSemaphores[currentFrame],
			VK_NULL_HANDLE, &imageIndex
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


		// 4. Reset and record command buffer
		vkResetCommandBuffer(m_commandBuffers[currentFrame], 0);
		RecordCommandBuffer(m_commandBuffers[currentFrame], imageIndex);

		// 5. Submit command buffer
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

		m_imageLayouts[imageIndex] = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

		// 6. Present
		VkPresentInfoKHR presentInfo = {};
		presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
		presentInfo.waitSemaphoreCount = 1;
		presentInfo.pWaitSemaphores = signalSemaphores;
		VkSwapchainKHR swapChains[] = { m_swapchain };
		presentInfo.swapchainCount = 1;
		presentInfo.pSwapchains = swapChains;
		presentInfo.pImageIndices = &imageIndex;

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

	void VulkanRenderer2D::RecordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex)
	{
		VulkanContext* vulkanContext = VulkanContext::Get();

		VkImageLayout currentLayout = m_imageLayouts[imageIndex];
		VkImage swapchainImage = vulkanContext->GetVulkanSwapchain().GetSwapchainImage(imageIndex);
		VkImage gameColorImage = vulkanContext->GetVulkanSwapchain().GetGameImage(imageIndex);

		VkCommandBufferBeginInfo beginInfo{};
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

		if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS)
		{
			EE_CORE_ERROR("Failed to begin recording command buffer!");
			return;
		}

		// === [0] Transition gameColorImage to COLOR_ATTACHMENT_OPTIMAL ===
		if (m_gameColorLayouts[imageIndex] != VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL)
		{
			VkImageMemoryBarrier toColorAttachment{};
			toColorAttachment.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
			toColorAttachment.oldLayout = m_gameColorLayouts[imageIndex];
			toColorAttachment.newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
			toColorAttachment.srcAccessMask = 0;
			toColorAttachment.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
			toColorAttachment.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			toColorAttachment.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			toColorAttachment.image = gameColorImage;
			toColorAttachment.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			toColorAttachment.subresourceRange.baseMipLevel = 0;
			toColorAttachment.subresourceRange.levelCount = 1;
			toColorAttachment.subresourceRange.baseArrayLayer = 0;
			toColorAttachment.subresourceRange.layerCount = 1;

			vkCmdPipelineBarrier(
				commandBuffer,
				VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
				VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
				0,
				0, nullptr,
				0, nullptr,
				1, &toColorAttachment
			);

			m_gameColorLayouts[imageIndex] = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
		}

		// === [1] Begin Game Render Pass ===
		VkRenderPassBeginInfo gameRenderPassInfo{};
		gameRenderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		gameRenderPassInfo.renderPass = vulkanContext->GetRenderPass();
		gameRenderPassInfo.framebuffer = vulkanContext->GetVulkanSwapchain().GetGameFramebuffer(imageIndex);
		gameRenderPassInfo.renderArea.offset = { 0, 0 };
		gameRenderPassInfo.renderArea.extent = vulkanContext->GetVulkanSwapchain().GetSwapchainExtent();

		VkClearValue gameClearColor = { {{0.1f, 0.1f, 0.1f, 1.0f}} };
		gameRenderPassInfo.clearValueCount = 1;
		gameRenderPassInfo.pClearValues = &gameClearColor;

		vkCmdBeginRenderPass(commandBuffer, &gameRenderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

		vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_vulkanGraphicsPipeline->GetPipeline());

		VkViewport viewport = { 0.0f, 0.0f, static_cast<float>(m_swapchainExtent.width), static_cast<float>(m_swapchainExtent.height), 0.0f, 1.0f };
		vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

		VkRect2D scissor = { {0, 0}, m_swapchainExtent };
		vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

		VkBuffer vertexBuffers[] = { s_VulkanData.QuadVertexBuffer->GetBuffer() };
		VkDeviceSize offsets[] = { 0 };
		vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);
		vkCmdBindIndexBuffer(commandBuffer, s_VulkanData.QuadIndexBuffer->GetBuffer(), 0, VK_INDEX_TYPE_UINT32);

		VkDescriptorSet descriptorSet = m_vulkanGraphicsPipeline->GetDescriptorSet(imageIndex);
		vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_vulkanGraphicsPipeline->GetPipelineLayout(), 0, 1, &descriptorSet, 0, nullptr);

		vkCmdDrawIndexed(commandBuffer, s_VulkanData.QuadIndexCount, 1, 0, 0, 0);
		vkCmdEndRenderPass(commandBuffer);

		// === [2] Transition gameColorImage to SHADER_READ_ONLY_OPTIMAL ===
		if (m_gameColorLayouts[imageIndex] != VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
		{
			VkImageMemoryBarrier toShaderRead{};
			toShaderRead.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
			toShaderRead.oldLayout = m_gameColorLayouts[imageIndex];
			toShaderRead.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			toShaderRead.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
			toShaderRead.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
			toShaderRead.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			toShaderRead.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			toShaderRead.image = gameColorImage;
			toShaderRead.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			toShaderRead.subresourceRange.baseMipLevel = 0;
			toShaderRead.subresourceRange.levelCount = 1;
			toShaderRead.subresourceRange.baseArrayLayer = 0;
			toShaderRead.subresourceRange.layerCount = 1;

			vkCmdPipelineBarrier(
				commandBuffer,
				VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
				VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
				0,
				0, nullptr,
				0, nullptr,
				1, &toShaderRead
			);

			m_gameColorLayouts[imageIndex] = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		}

		// === [3] ImGui Render Pass ===
		VkRenderPassBeginInfo imguiRenderPassInfo{};
		imguiRenderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		imguiRenderPassInfo.renderPass = vulkanContext->GetImGuiRenderPass();
		imguiRenderPassInfo.framebuffer = vulkanContext->GetVulkanSwapchain().GetImGuiFramebuffer(imageIndex);
		imguiRenderPassInfo.renderArea.offset = { 0, 0 };
		imguiRenderPassInfo.renderArea.extent = vulkanContext->GetVulkanSwapchain().GetSwapchainExtent();
		imguiRenderPassInfo.clearValueCount = 1;
		imguiRenderPassInfo.pClearValues = &gameClearColor;

		vkCmdBeginRenderPass(commandBuffer, &imguiRenderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
		ImDrawData* drawData = ImGui::GetDrawData();
		if (drawData) {
			ImGui_ImplVulkan_RenderDrawData(drawData, commandBuffer);
		}
		vkCmdEndRenderPass(commandBuffer);

		// === [4] Transition swapchain image to PRESENT_SRC_KHR ===
		if (m_imageLayouts[imageIndex] != VK_IMAGE_LAYOUT_PRESENT_SRC_KHR)
		{
			VkImageMemoryBarrier toPresent{};
			toPresent.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
			toPresent.oldLayout = m_imageLayouts[imageIndex];
			toPresent.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
			toPresent.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
			toPresent.dstAccessMask = 0;
			toPresent.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			toPresent.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			toPresent.image = swapchainImage;
			toPresent.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			toPresent.subresourceRange.baseMipLevel = 0;
			toPresent.subresourceRange.levelCount = 1;
			toPresent.subresourceRange.baseArrayLayer = 0;
			toPresent.subresourceRange.layerCount = 1;

			vkCmdPipelineBarrier(
				commandBuffer,
				VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
				VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
				0,
				0, nullptr,
				0, nullptr,
				1, &toPresent
			);

			m_imageLayouts[imageIndex] = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
		}

		if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS)
		{
			EE_CORE_ASSERT(false, "Failed to record command buffer!");
		}
	}





	// Transition the image layout from PRESENT_SRC_KHR to SHADER_READ_ONLY_OPTIMAL
	void VulkanRenderer2D::TransitionImageForShaderRead(VkCommandBuffer cmd, uint32_t imageIndex, VkImage image, VulkanContext* vulkanContext)
	{
		VkImageMemoryBarrier barrier = {};
		barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		barrier.oldLayout = m_imageLayouts[imageIndex];  // Previous layout, which is PRESENT_SRC_KHR
		barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;  // New layout for shader read access
		barrier.srcAccessMask = 0;  // No source access mask required
		barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;  // We want to read from the image in the shader
		barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barrier.image = image;
		barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;  // Color aspect
		barrier.subresourceRange.baseMipLevel = 0;
		barrier.subresourceRange.levelCount = 1;
		barrier.subresourceRange.baseArrayLayer = 0;
		barrier.subresourceRange.layerCount = 1;

		// Insert the barrier into the command buffer to synchronize the transition
		vkCmdPipelineBarrier(
			cmd,
			VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,  // Wait for previous work to finish (may not be strictly necessary)
			VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,  // Ensure the image is ready for fragment shader access
			0,
			0, nullptr,
			0, nullptr,
			1, &barrier
		);

		// Update the image layout tracker
		m_imageLayouts[imageIndex] = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
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


	void VulkanRenderer2D::DrawTextureQuad(const glm::mat4& transform, const std::shared_ptr<VulkanTexture>& texture, float tilingFactor, const glm::vec4& tintColor)
	{
		//Engine loads all the texture to AssetManger. Game layer Gets() those textures from AssetManager 
		// map and sends Ref through Draw(texture). Draw() then finds matching texture slot here.

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
		

		// move these
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

	void VulkanRenderer2D::DrawQuad(const glm::mat4& transform, const glm::vec4& color, int entityID)
	{
		if (s_VulkanData.QuadIndexCount >= VulkanRenderer2DData::MaxIndices)
		{
			EE_CORE_WARN("Quad index limit reached!");
			return;
		}

		constexpr size_t quadVertexCount = 4;
		constexpr glm::vec2 textureCoords[] = {
			{ 0.0f, 0.0f },
			{ 1.0f, 0.0f },
			{ 1.0f, 1.0f },
			{ 0.0f, 1.0f }
		};

		const float textureIndex = 0.0f; // White Texture
		const float tilingFactor = 1.0f;

		// Extract translation, rotation, and scale from transform
		glm::vec3 translation = glm::vec3(transform[3]); // Last column
		glm::vec3 scale = {
			glm::length(glm::vec3(transform[0])),
			glm::length(glm::vec3(transform[1])),
			glm::length(glm::vec3(transform[2]))
		};

		// Build rotation + translation matrix (without scale)
		glm::mat4 rotationTranslation = transform;
		rotationTranslation[0] = glm::normalize(glm::vec4(glm::vec3(transform[0]), 0.0f));
		rotationTranslation[1] = glm::normalize(glm::vec4(glm::vec3(transform[1]), 0.0f));
		rotationTranslation[2] = glm::normalize(glm::vec4(glm::vec3(transform[2]), 0.0f));
		rotationTranslation[3] = glm::vec4(translation, 1.0f);

		for (size_t i = 0; i < quadVertexCount; i++)
		{
			glm::vec3 scaledPosition = s_VulkanData.QuadVertexPositions[i];
			scaledPosition.x *= scale.x;
			scaledPosition.y *= scale.y;

			s_VulkanData.QuadVertexBufferPtr->Position = rotationTranslation * glm::vec4(scaledPosition, 1.0f);
			s_VulkanData.QuadVertexBufferPtr->Color = color;
			s_VulkanData.QuadVertexBufferPtr->TexCoord = textureCoords[i];
			s_VulkanData.QuadVertexBufferPtr->TexIndex = textureIndex;
			s_VulkanData.QuadVertexBufferPtr->TilingFactor = tilingFactor;
			s_VulkanData.QuadVertexBufferPtr++;
		}

		s_VulkanData.QuadIndexCount += 6;
		s_VulkanData.Stats.QuadCount++;
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

	void VulkanRenderer2D::CreateImGuiTextureDescriptors()
	{
		auto& swapchain = m_vulkanContext->GetVulkanSwapchain();
		auto imageViews = swapchain.GetGameColorAttachmentImageViews();

		m_gameViewportDescriptorSets.resize(imageViews.size());

		for (size_t i = 0; i < imageViews.size(); ++i)
		{
			m_gameViewportDescriptorSets[i] = ImGui_ImplVulkan_AddTexture(
				m_vulkanContext->GetSampler(),
				imageViews[i],
				VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
			);
		}
	}

	
	
}
