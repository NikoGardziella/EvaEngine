#include "pch.h"
#include "VulkanRenderer2D.h"
#include "Engine/Platform/Vulkan/VulkanContext.h"
#include <Engine/Platform/Vulkan/VulkanGraphicsPipeline.h>
#include "VertexArray.h"
#include "Shader.h"
#include "Renderer2D.h"
#include "OrthographicCameraController.h"
#include "Renderer.h"
#include <backends/imgui_impl_vulkan.h>

namespace Engine {


	



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
		m_vulkanGraphicsPipeline = std::make_shared<VulkanGraphicsPipeline>(m_vulkanContext->GetDeviceManager().GetDevice(),
			m_vulkanContext->GetVulkanSwapchain().GetSwapchainExtent(), m_vulkanContext->GetRenderPass());
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
		m_vertexBuffer = std::make_shared<VulkanVertexBuffer>((float*)quadVertices.data(), sizeof(VulkanQuadVertex) * quadVertices.size());

		EE_CORE_INFO("Vertex buffer created with size: {}", sizeof(VulkanQuadVertex) * quadVertices.size());

		
		m_indexBuffer = std::make_shared<VulkanIndexBuffer>(quadIndices.data(), sizeof(VulkanQuadVertex) * quadVertices.size());
		EE_CORE_INFO("Index buffer created with size: {}", sizeof(uint32_t) * quadVertices.size());

		m_camera = std::make_shared<OrthographicCamera>(-5.0f, 5.0f, -5.0f, 5.0f);
		m_camera->SetPosition({ 0.0f, 0.0f, 0.0f }); // Move the camera back to see the quad

		// Update the uniform buffer with the camera's view-projection matrix
		m_vulkanGraphicsPipeline->UpdateUniformBuffer(m_camera->GetViewProjectionMatrix());
	
	}



	void VulkanRenderer2D::DrawFrame(uint32_t currentFrame)
	{
		m_vulkanGraphicsPipeline->UpdateUniformBuffer(m_camera->GetViewProjectionMatrix());


		vkWaitForFences(m_device, 1, &m_inFlightFences[currentFrame], VK_TRUE, UINT64_MAX);
		vkResetFences(m_device, 1, &m_inFlightFences[currentFrame]);
		
		m_vulkanGraphicsPipeline->UpdateDescriptorSets(currentFrame);

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

		VkClearValue clearColor = { {{0.9f, 0.7f, 0.7f, 1.0f}} };
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

		VkBuffer vertexBuffers[] = { m_vertexBuffer->GetBuffer() };
		VkDeviceSize offsets[] = { 0 };
		vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);
		vkCmdBindIndexBuffer(commandBuffer, m_indexBuffer->GetBuffer(), 0, VK_INDEX_TYPE_UINT32);

		VkDescriptorSet descriptorSet = m_vulkanGraphicsPipeline->GetDescriptorSet(imageIndex);
		
	 
		vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_vulkanGraphicsPipeline->GetPipelineLayout(),
			0, 1, &descriptorSet, 0, nullptr);

		vkCmdDrawIndexed(commandBuffer, static_cast<uint32_t>(quadIndices.size()), 1, 0, 0, 0);

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
		imguiRenderPassInfo.clearValueCount = 1;
		imguiRenderPassInfo.pClearValues = &imguiClearValue;

		vkCmdBeginRenderPass(commandBuffer, &imguiRenderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

		// === ImGui rendering ===
		ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), commandBuffer);

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

	


	
}
