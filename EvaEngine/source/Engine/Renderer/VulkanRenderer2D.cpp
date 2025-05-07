#include "pch.h"
#include "VulkanRenderer2D.h"
#include "Engine/Platform/Vulkan/VulkanContext.h"
#include <Engine/Platform/Vulkan/VulkanGraphicsPipeline.h>
#include "Engine/AssetManager/AssetManager.h"
#include "Engine/Platform/Vulkan/VulkanUtils.h"


#include "VertexArray.h"
#include "Shader.h"
#include "OrthographicCameraController.h"
#include "Renderer.h"
#include <backends/imgui_impl_vulkan.h>

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/string_cast.hpp>
#include <glm/gtx/matrix_decompose.hpp>

namespace Engine {

	//VulkanRenderer2D::SceneData* VulkanRenderer2D::m_sceneData = new SceneData();

	
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
		std::array<Ref<VulkanPixelTexture>, MaxTextureSlots> PixelTextureSlots;
		uint32_t TextureSlotIndex = 1; // 0 = white texture

		glm::vec3 QuadVertexPositions[4];


		Renderer2D::Statistics Stats;

		struct CameraData
		{
			glm::mat4 ViewProjection;
		};
		CameraData CameraBuffer;
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

		m_vulkanContext = VulkanContext::Get();
		m_swapchain = m_vulkanContext->GetVulkanSwapchain().GetSwapchain();
		m_swapchainExtent = m_vulkanContext->GetVulkanSwapchain().GetSwapchainExtent();
		m_vulkanGraphicsPipelines = std::make_shared<VulkanGraphicsPipeline>(*m_vulkanContext);
		m_device = m_vulkanContext->GetDeviceManager().GetDevice();

		// Allocate command buffers and sync objects
		AllocateCommandBuffers(m_vulkanContext->GetDeviceManager().GetDevice(), m_vulkanContext->GetCommandPool());
		CreateSyncObjects();


		m_camera = std::make_shared<OrthographicCamera>(-5.0f, 5.0f, -5.0f, 5.0f);
		m_camera->SetPosition({ 0.0f, 0.0f, 1.0f }); // Move the camera back to see the quad

		// Update the uniform buffer with the camera's view-projection matrix
		for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
		{
			m_vulkanGraphicsPipelines->UpdateUniformBuffer(i, m_camera->GetViewProjectionMatrix());
		}

		s_VulkanData.QuadVertexBufferBase = new VulkanQuadVertex[VulkanRenderer2DData::MaxVertices];
		s_VulkanData.QuadVertexBufferPtr = s_VulkanData.QuadVertexBufferBase;

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
		// Fill initial texture slots
		s_VulkanData.TextureSlots[0] = s_VulkanData.WhiteTexture;

		s_VulkanData.TextureSlots[1] = AssetManager::AddTexture("logo", Engine::AssetManager::GetAssetPath("textures/ee_logo.png").string());
		s_VulkanData.TextureSlotIndex++;

		s_VulkanData.TextureSlots[2] = AssetManager::AddTexture("chess", Engine::AssetManager::GetAssetPath("textures/chess_board.png").string());
		s_VulkanData.TextureSlotIndex++;

		s_VulkanData.TextureSlots[3] = AssetManager::AddPixelTexture("pixel", Engine::AssetManager::GetAssetPath("textures/pixel_texture1.png").string());
		s_VulkanData.TextureSlotIndex++;

		s_VulkanData.TextureSlots[4] = AssetManager::AddTexture("player", Engine::AssetManager::GetAssetPath("textures/Idle_gun_000.png").string());
		s_VulkanData.TextureSlotIndex++;

		s_VulkanData.TextureSlots[5] = AssetManager::AddTexture("bullet", Engine::AssetManager::GetAssetPath("textures/Fire_small_asset.png").string());
		s_VulkanData.TextureSlotIndex++;

		// Fill the rest of the slots with pixel texture
		for (uint32_t i = s_VulkanData.TextureSlotIndex; i < s_VulkanData.TextureSlots.size(); i++)
		{
			s_VulkanData.TextureSlots[i] = s_VulkanData.WhiteTexture;
			s_VulkanData.TextureSlotIndex++;
		}

		for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
		{
			m_vulkanGraphicsPipelines->UpdateTrackedImageDescriptorSets(i, s_VulkanData.TextureSlots);
		}

		// this is for rendering game in editor viewport
		CreateImGuiTextureDescriptors();	
	}

	void VulkanRenderer2D::BeginFrame(uint32_t currentFrame)
	{
		VkCommandBuffer cmd = m_commandBuffers[currentFrame];


		
		// 1. Sync
		vkWaitForFences(m_device, 1, &m_inFlightFences[currentFrame], VK_TRUE, UINT64_MAX);
		vkResetFences(m_device, 1, &m_inFlightFences[currentFrame]);

		m_vulkanGraphicsPipelines->UpdateUniformBuffer(currentFrame, s_VulkanData.CameraBuffer.ViewProjection);

		// 2. Acquire. Max current frame is 2 and max swapchain images is 3.
		// set in Renderer.h 	const int MAX_FRAMES_IN_FLIGHT = 2;
		
		VkResult result = vkAcquireNextImageKHR(m_device, m_swapchain, UINT64_MAX, m_imageAvailableSemaphores[currentFrame], VK_NULL_HANDLE, &m_imageIndex);

		if (result == VK_ERROR_OUT_OF_DATE_KHR)
		{
			m_vulkanContext->GetVulkanSwapchain().RecreateSwapchain();
			return;
		}
		else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR)
		{
			EE_CORE_ASSERT(false, "Failed to acquire swapchain image!");
		}

		// 3. Record Game Pass
		vkResetCommandBuffer(cmd, 0);

		VkCommandBufferBeginInfo beginInfo{};
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

		vkBeginCommandBuffer(cmd, &beginInfo);


		//From RecordGameCommand()

		// --- Begin render pass ---
		VkRenderPassBeginInfo renderPassInfo{};
		renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		renderPassInfo.renderPass = m_vulkanContext->GetGameRenderPass();
		renderPassInfo.framebuffer = m_vulkanContext->GetVulkanSwapchain().GetGameFramebuffer(m_imageIndex);
		renderPassInfo.renderArea = { {0, 0}, m_vulkanContext->GetVulkanSwapchain().GetSwapchainExtent() };

		// Clear color for the color attachment
		VkClearValue clearColor = { {0.8f, 0.2f, 0.35f, 1.0f} };
		renderPassInfo.clearValueCount = 1;
		renderPassInfo.pClearValues = &clearColor;

		vkCmdBeginRenderPass(cmd, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

		// --- Set viewport and scissor ---
		VkViewport viewport = {};
		viewport.width = static_cast<float>(m_vulkanContext->GetVulkanSwapchain().GetSwapchainExtent().width);
		viewport.height = static_cast<float>(m_vulkanContext->GetVulkanSwapchain().GetSwapchainExtent().height);
		viewport.minDepth = 0.0f;
		viewport.maxDepth = 1.0f;
		vkCmdSetViewport(cmd, 0, 1, &viewport);

		VkRect2D scissor = { {0, 0}, m_vulkanContext->GetVulkanSwapchain().GetSwapchainExtent() };
		vkCmdSetScissor(cmd, 0, 1, &scissor);

		// --- Bind pipeline and draw ---
		vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_vulkanGraphicsPipelines->GetGamePipeline());


	}

	void VulkanRenderer2D::EndFrame(uint32_t currentFrame)
	{
		m_firstIndex = 0;
		m_vertexOffset = 0;
		VkCommandBuffer cmd = m_commandBuffers[currentFrame];
		// End RecordGameDrawCommands render pass
		vkCmdEndRenderPass(cmd);


		RecordPresentDrawCommands(cmd, m_imageIndex, currentFrame);

		RecordEditorDrawCommands(cmd, m_imageIndex);

		
		// RecordImGuiDrawCommands(cmd, imageIndex);
		vkEndCommandBuffer(cmd);

		// 4. Submit
		VkSemaphore waitSemaphores[] = { m_imageAvailableSemaphores[currentFrame] };
		VkSemaphore signalSemaphores[] = { m_renderFinishedSemaphores[currentFrame] };
		VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };

		VkSubmitInfo submitInfo{};
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		submitInfo.waitSemaphoreCount = 1;
		submitInfo.pWaitSemaphores = waitSemaphores;
		submitInfo.pWaitDstStageMask = waitStages;
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &cmd;
		submitInfo.signalSemaphoreCount = 1;
		submitInfo.pSignalSemaphores = signalSemaphores;

		if (vkQueueSubmit(m_vulkanContext->GetGraphicsQueue(), 1, &submitInfo, m_inFlightFences[currentFrame]) != VK_SUCCESS)
		{
			EE_CORE_ASSERT(false, "Failed to submit draw command buffer!");
		}

		// 5. Present
		VkPresentInfoKHR presentInfo{};
		presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
		presentInfo.waitSemaphoreCount = 1;
		presentInfo.pWaitSemaphores = signalSemaphores;
		VkSwapchainKHR swapChains[] = { m_swapchain };
		presentInfo.swapchainCount = 1;
		presentInfo.pSwapchains = swapChains;
		presentInfo.pImageIndices = &m_imageIndex;

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

	void VulkanRenderer2D::DeviceWaitIdle()
	{
		vkDeviceWaitIdle(m_device);
	}

	void VulkanRenderer2D::StartBatch()
	{
		s_VulkanData.QuadVertexBufferPtr = s_VulkanData.QuadVertexBufferBase;
		s_VulkanData.QuadIndexCount = 0;
	}

	void VulkanRenderer2D::NextBatch()
	{
		StartBatch();
		Flush();
	}

	void VulkanRenderer2D::Flush()
	{
		Renderer::DrawFrame();
	}

	void VulkanRenderer2D::DrawFrame(uint32_t currentFrame)
	{
		//this can be called multiple times per frame
		VkCommandBuffer cmd = m_commandBuffers[currentFrame];
		RecordGameDrawCommands(cmd, m_imageIndex, currentFrame);

		s_VulkanData.Stats.DrawCalls++;

	}

	void VulkanRenderer2D::RecordGameDrawCommands(VkCommandBuffer commandBuffer, uint32_t imageIndex, uint32_t currentFrame)
	{
		EE_PROFILE_FUNCTION();


		VkBuffer vertexBuffers[] = { s_VulkanData.QuadVertexBuffer->GetBuffer() };
		VkDeviceSize offsets[] = { 0 };
		vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);
		vkCmdBindIndexBuffer(commandBuffer, s_VulkanData.QuadIndexBuffer->GetBuffer(), 0, VK_INDEX_TYPE_UINT32);

		VkDescriptorSet descriptorSet = m_vulkanGraphicsPipelines->GetGameDescriptorSet(currentFrame);
		vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_vulkanGraphicsPipelines->GetGamePipelineLayout(), 1, 1, &descriptorSet, 0, nullptr);

		descriptorSet = m_vulkanGraphicsPipelines->GetCameraDescriptorSet(currentFrame);
		vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_vulkanGraphicsPipelines->GetGamePipelineLayout(), 0, 1, &descriptorSet, 0, nullptr);

		vkCmdDrawIndexed(commandBuffer, s_VulkanData.QuadIndexCount, 1, 0, 0, 0);

		m_firstIndex += s_VulkanData.QuadIndexCount;
		m_vertexOffset += (s_VulkanData.QuadIndexCount / 6) * 4;

	}


	void VulkanRenderer2D::RecordPresentDrawCommands(VkCommandBuffer commandBuffer, uint32_t imageIndex, uint32_t currentFrame)
	{
		EE_PROFILE_FUNCTION();

		// Begin render pass to the swapchain (present) framebuffer
		VkRenderPassBeginInfo renderPassInfo{};
		renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		renderPassInfo.renderPass = m_vulkanContext->GetPresentRenderPass();
		renderPassInfo.framebuffer = m_vulkanContext->GetVulkanSwapchain().GetSwapchainFramebuffer(imageIndex); // Assuming same framebuffer for simplicity
		renderPassInfo.renderArea.offset = { 0, 0 };
		renderPassInfo.renderArea.extent = m_vulkanContext->GetVulkanSwapchain().GetSwapchainExtent();

		VkClearValue clearColor = { {0.0f, 0.0f, 0.9f, 1.0f} };
		renderPassInfo.clearValueCount = 1;
		renderPassInfo.pClearValues = &clearColor;

		vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

		// Setup viewport and scissor
		VkViewport viewport{};
		viewport.x = 0.0f;
		viewport.y = 0.0f;
		viewport.width = static_cast<float>(m_vulkanContext->GetVulkanSwapchain().GetSwapchainExtent().width);
		viewport.height = static_cast<float>(m_vulkanContext->GetVulkanSwapchain().GetSwapchainExtent().height);  // Negative height for flip
		viewport.minDepth = 0.0f;
		viewport.maxDepth = 1.0f;
		vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

		VkRect2D scissor{};
		scissor.offset = { 0, 0 };
		scissor.extent = m_vulkanContext->GetVulkanSwapchain().GetSwapchainExtent();
		vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

		// Bind the graphics pipeline for the game scene
		vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_vulkanGraphicsPipelines->GetPresentPipeline());

		VkDescriptorSet descriptorSet = m_vulkanGraphicsPipelines->GetPresentDescriptorSet(currentFrame);
		vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_vulkanGraphicsPipelines->GetPresentPipelineLayout(), 0, 1, &descriptorSet, 0, nullptr);

		// hardcoded vertices in fullscreen_shader:
		vkCmdDraw(commandBuffer, 3, 1, 0, 0);

		vkCmdEndRenderPass(commandBuffer);

	}

	void VulkanRenderer2D::RecordEditorDrawCommands(VkCommandBuffer commandBuffer, uint32_t imageIndex)
	{
		EE_PROFILE_FUNCTION();



		// Begin ImGui render pass
		VkRenderPassBeginInfo imguiRenderPassInfo{};
		imguiRenderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		imguiRenderPassInfo.renderPass = m_vulkanContext->GetImGuiRenderPass();
		imguiRenderPassInfo.framebuffer = m_vulkanContext->GetVulkanSwapchain().GetImGuiFramebuffer(imageIndex);
		imguiRenderPassInfo.renderArea.offset = { 0, 0 };
		imguiRenderPassInfo.renderArea.extent = m_vulkanContext->GetVulkanSwapchain().GetSwapchainExtent();

		VkClearValue clearValue{};
		clearValue.color = { {0.0f, 0.9f, 0.0f, 0.0f} };
		imguiRenderPassInfo.clearValueCount = 1;
		imguiRenderPassInfo.pClearValues = &clearValue;

		vkCmdBeginRenderPass(commandBuffer, &imguiRenderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

		ImDrawData* imguiDrawData = ImGui::GetDrawData();
		if (imguiDrawData != nullptr)
		{

			ImGui_ImplVulkan_RenderDrawData(imguiDrawData, commandBuffer);
		}

		vkCmdEndRenderPass(commandBuffer);

	}


	// Not uset for now. Maybe later
	void VulkanRenderer2D::TransitionImageLayout(VkCommandBuffer commandBuffer,	VkImage image,
		VkImageLayout oldLayout, VkImageLayout newLayout)
	{
		VkImageMemoryBarrier barrier{};
		barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		barrier.oldLayout = oldLayout;
		barrier.newLayout = newLayout;
		barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barrier.image = image;
		barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		barrier.subresourceRange.baseMipLevel = 0;
		barrier.subresourceRange.levelCount = 1;
		barrier.subresourceRange.baseArrayLayer = 0;
		barrier.subresourceRange.layerCount = 1;

		//*********** src VkAccessFlags *****************
		// flag indicates which types of access to the image (or buffer) are 
		// required by the pipeline before the layout transition.
		// defines which operations or stages (such as reading or writing) need
		// to happen on the image before the layout change.
		// - VK_ACCESS_SHADER_READ_BIT: The image will be read by a shader.
		// - VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT: The image will be written to as a color attachment.
		// - VK_ACCESS_MEMORY_READ_BIT : General memory read access(for non - shader access).
		// - VK_ACCESS_MEMORY_WRITE_BIT : General memory write access.
		VkAccessFlags srcAccessMask = 0;

		//*********** dst VkAccessFlags *****************
		//  indicates the type of access after the layout transition has been completed.
		// defines the operations that will need access to the image in the new layout.
		VkAccessFlags dstAccessMask = 0;

		//************ sourceStage (VkPipelineStageFlags) *********
		//  specifies the pipeline stage during which the source access 
		// (specified by srcAccessMask) will occur before the layout transition.
		// ensures that the pipeline has finished all operations that 
		// need to occur before the transition
		VkPipelineStageFlags sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;

		// ******** destinationStage (VkPipelineStageFlags) ********
		// pecifies the pipeline stage after the layout transition, during which
		// the destination access (specified by dstAccessMask) will occur.
		// -VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT: Used when no specific stage is required.
		// -VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT: Used when you need to output to a color attachment.
		// -VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT : Used when you want to access the resource in a fragment shader.
		// -VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT : Used when you want to access the resource in a compute shader.
		VkPipelineStageFlags destinationStage = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;



		if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
		{
			dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
			destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
		}
		else if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL)
		{
			dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
			destinationStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		}
		else if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL)
		{
			dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
			destinationStage = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
		}
		else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
		{
			srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
			dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
			sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
			destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
		}
		else if (oldLayout == VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_PRESENT_SRC_KHR)
		{
			// Transition from COLOR_ATTACHMENT_OPTIMAL to PRESENT_SRC_KHR (for presentation)
			barrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
			barrier.dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
			sourceStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
			destinationStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
		}
		else if (oldLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL)
		{
			srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
			dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
			sourceStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
			destinationStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		}
		else if (oldLayout == VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
		{
			srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
			dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
			sourceStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
			destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
		}
		else if (oldLayout == VK_IMAGE_LAYOUT_PRESENT_SRC_KHR && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
		{
			srcAccessMask = VK_ACCESS_MEMORY_READ_BIT;
			dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
			sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
			destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
		}
		else if (oldLayout == VK_IMAGE_LAYOUT_PRESENT_SRC_KHR && newLayout == VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL)
		{
			// Transition from PRESENT_SRC_KHR to COLOR_ATTACHMENT_OPTIMAL
			barrier.srcAccessMask = VK_ACCESS_MEMORY_READ_BIT; // Memory read during presentation
			barrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT; // Write access for color attachment
			sourceStage = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT; // After presentation
			destinationStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT; // Before rendering
		}
		else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL)
		{
			srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
			dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
			sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
			destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
		}
		else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
		{
			srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
			dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
			sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
			destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
		}
		else if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_GENERAL)
		{
			dstAccessMask = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT;
			destinationStage = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
		}
		else if (oldLayout == VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
			// Transition from COLOR_ATTACHMENT_OPTIMAL to SHADER_READ_ONLY_OPTIMAL
			barrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
			barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
			sourceStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
			destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
		}
		else if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_PRESENT_SRC_KHR) {
			// Transition from UNDEFINED to PRESENT_SRC_KHR (required for presenting)
			barrier.srcAccessMask = 0;
			barrier.dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
			sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
			destinationStage = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
		}
		else if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL) 
		{
			// Transition from UNDEFINED to COLOR_ATTACHMENT_OPTIMAL (for rendering)
			barrier.srcAccessMask = 0;
			barrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
			sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
			destinationStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		}
		else
		{
			EE_CORE_ERROR("Unsupported layout transition: {} -> {}",
				VulkanUtils::LayoutToString(oldLayout),
				VulkanUtils::LayoutToString(newLayout));
		}

		barrier.srcAccessMask = srcAccessMask;
		barrier.dstAccessMask = dstAccessMask;


		// vkCmdPipelineBarrier command ensures that the proper synchronization occurs between
		// different stages of the Vulkan pipeline by specifying how and when the image will be used.

		//std::string layoutold = VulkanUtils::LayoutToString(oldLayout);
		//std::string layoutnew = VulkanUtils::LayoutToString(newLayout);

		EE_CORE_INFO("Transitioning layout from {} to {}", VulkanUtils::LayoutToString(oldLayout), VulkanUtils::LayoutToString(newLayout));

		vkCmdPipelineBarrier(
			commandBuffer,
			sourceStage,
			destinationStage,
			0,
			0, nullptr,
			0, nullptr,
			1, &barrier
		);
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

		if (s_VulkanData.QuadIndexCount >= VulkanRenderer2DData::MaxIndices)
		{
			// I did not know how to handle this the way it happened in openGL
			// RecordGameDrawCommands() overwrites previous data. I tried to remove clear
			// but then the old data will be drawn as well.
			// Possible solution is to create new dataBuffer and draw all the dataBufers 
			// at the same time. Didnt want to implement it yet
			EE_CORE_ASSERT(false, "Quad index count exceeded maximum limit!");

		}
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
			NextBatch();
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


	void VulkanRenderer2D::BeginScene(const Camera& camera, const glm::mat4& transform)
	{
		EE_PROFILE_FUNCTION();
		//StartBatch();
		s_VulkanData.CameraBuffer.ViewProjection = camera.GetViewProjection() * glm::inverse(transform);
		//s_VulkanData.CameraBuffer.SetData(&s_Data.CameraBuffer, sizeof(Renderer2DData::CameraData));
		StartBatch();

	}

	void VulkanRenderer2D::BeginScene(const EditorCamera& camera)
	{
		EE_PROFILE_FUNCTION();

		s_VulkanData.CameraBuffer.ViewProjection = camera.GetViewProjection();
		//s_VulkanData.CameraUniformBuffer->SetData(&s_VulkanData.CameraBuffer, sizeof(Renderer2DData::CameraData));
		StartBatch();
	}

	void VulkanRenderer2D::BeginScene(glm::mat4 viewProjectionMatrix)
	{
		s_VulkanData.CameraBuffer.ViewProjection = viewProjectionMatrix;
		StartBatch();
	}

	void VulkanRenderer2D::BeginScene()
	{
		StartBatch();
	}

	

	void VulkanRenderer2D::EndScene()
	{
		EE_PROFILE_FUNCTION();
		// Flush the batch
		uint32_t dataSize = (uint32_t)((uint8_t*)s_VulkanData.QuadVertexBufferPtr - (uint8_t*)s_VulkanData.QuadVertexBufferBase);
		if (dataSize > 0)
		{
			s_VulkanData.QuadVertexBuffer->SetData(s_VulkanData.QuadVertexBufferBase, dataSize);
			Flush();
		}
		
	}

	
	void VulkanRenderer2D::CreateImGuiTextureDescriptors()
	{
		// this is for rendering game in editor viewport
		auto& swapchain = m_vulkanContext->GetVulkanSwapchain();
		std::vector<VulkanTracked>& imageViews = swapchain.GetGameTrackedImages();

		m_gameViewportDescriptorSets.resize(imageViews.size());

		for (size_t i = 0; i < imageViews.size(); ++i)
		{
			m_gameViewportDescriptorSets[i] = ImGui_ImplVulkan_AddTexture(
				m_vulkanContext->GetSampler(),
				imageViews[i].view,
				VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
			);
		}
	}

	Renderer2D::Statistics VulkanRenderer2D::GetStats()
	{
		return s_VulkanData.Stats;
	}

	void VulkanRenderer2D::ResetStats()
	{
		memset(&s_VulkanData.Stats, 0, sizeof(Renderer2D::Statistics));
	}
}
