#include "pch.h"
#include "VulkanRenderer2D.h"
#include "Engine/Platform/Vulkan/VulkanContext.h"
#include <Engine/Platform/Vulkan/VulkanGraphicsPipeline.h>
#include "VertexArray.h"
#include "Shader.h"
#include "Renderer2D.h"
#include "OrthographicCameraController.h"

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



	void VulkanRenderer2D::DrawFrame()
	{
		m_vulkanGraphicsPipeline->UpdateUniformBuffer(m_camera->GetViewProjectionMatrix());

		// Wait for the fence to ensure the previous frame has finished
		vkWaitForFences(m_device, 1, &m_inFlightFences[m_currentFrame], VK_TRUE, UINT64_MAX);

		// Reset the fence before submitting new work
		vkResetFences(m_device, 1, &m_inFlightFences[m_currentFrame]);

		// Update the descriptor set after ensuring the previous frame has completed
		m_vulkanGraphicsPipeline->UpdateDescriptorSets(m_currentFrame);

		// Acquire next image from swapchain
		VkResult result = vkAcquireNextImageKHR(m_device, m_swapchain, UINT64_MAX,
			m_imageAvailableSemaphores[m_currentFrame],
			VK_NULL_HANDLE, &m_currentFrame);

		// Handle swapchain recreation if needed
		if (result == VK_ERROR_OUT_OF_DATE_KHR)
		{
			m_vulkanContext->GetVulkanSwapchain().RecreateSwapchain();
			return;
		}
		else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR)
		{
			EE_CORE_ASSERT(false, "Failed to acquire swapchain image!");
		}

		// Reset command buffer
		vkResetCommandBuffer(m_commandBuffers[m_currentFrame], 0);

		// Record command buffer
		RecordCommandBuffer(m_commandBuffers[m_currentFrame], m_currentFrame);

		// Submit command buffer
		VkSubmitInfo submitInfo{};
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

		VkSemaphore waitSemaphores[] = { m_imageAvailableSemaphores[m_currentFrame] };
		VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
		submitInfo.waitSemaphoreCount = 1;
		submitInfo.pWaitSemaphores = waitSemaphores;
		submitInfo.pWaitDstStageMask = waitStages;

		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &m_commandBuffers[m_currentFrame];

		VkSemaphore signalSemaphores[] = { m_renderFinishedSemaphores[m_currentFrame] };
		submitInfo.signalSemaphoreCount = 1;
		submitInfo.pSignalSemaphores = signalSemaphores;

		if (vkQueueSubmit(m_vulkanContext->GetGraphicsQueue(), 1, &submitInfo, m_inFlightFences[m_currentFrame]) != VK_SUCCESS)
		{
			EE_CORE_ASSERT(false, "Failed to submit draw command buffer!");
		}

		// Present the image
		VkPresentInfoKHR presentInfo{};
		presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
		presentInfo.waitSemaphoreCount = 1;
		presentInfo.pWaitSemaphores = signalSemaphores;

		VkSwapchainKHR swapChains[] = { m_swapchain };
		presentInfo.swapchainCount = 1;
		presentInfo.pSwapchains = swapChains;
		presentInfo.pImageIndices = &m_currentFrame;

		VkResult presentResult = vkQueuePresentKHR(m_vulkanContext->GetGraphicsQueue(), &presentInfo);

		if (presentResult == VK_ERROR_OUT_OF_DATE_KHR || presentResult == VK_SUBOPTIMAL_KHR)
		{
			m_vulkanContext->GetVulkanSwapchain().RecreateSwapchain();
		}
		else if (presentResult != VK_SUCCESS)
		{
			EE_CORE_ASSERT(false, "Failed to present swapchain image!");
		}

		// Advance to the next frame
		m_currentFrame = (m_currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
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
		beginInfo.flags = 0; // Optional
		beginInfo.pInheritanceInfo = nullptr; // Optional

		if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS)
		{
			EE_CORE_ERROR("Failed to begin recording command buffer!");
		}

		VkRenderPassBeginInfo renderPassInfo{};
		renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		renderPassInfo.renderPass = m_vulkanContext->GetRenderPass();
		renderPassInfo.framebuffer = m_vulkanContext->GetSwapchainFramebuffer(imageIndex);
		renderPassInfo.renderArea.offset = { 0, 0 };
		renderPassInfo.renderArea.extent = m_vulkanContext->GetVulkanSwapchain().GetSwapchainExtent();

		VkClearValue clearColor = { {{0.9f, 0.7f, 0.7f, 1.0f}} };
		renderPassInfo.clearValueCount = 1;
		renderPassInfo.pClearValues = &clearColor;

		vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

		vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_vulkanGraphicsPipeline->GetPipeline());

		VkViewport viewport{};
		viewport.x = 0.0f;
		viewport.y = 0.0f;
		viewport.width = static_cast<float>(m_swapchainExtent.width);
		viewport.height = static_cast<float>(m_swapchainExtent.height);
		viewport.minDepth = 0.0f;
		viewport.maxDepth = 1.0f;
		vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

		VkRect2D scissor{};
		scissor.offset = { 0, 0 };
		scissor.extent = m_swapchainExtent;
		vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

		// Bind the vertex buffer
		VkBuffer vertexBuffers[] = { m_vertexBuffer->GetBuffer() };
		VkDeviceSize offsets[] = { 0 };
		vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);

		// Bind the index buffer
		vkCmdBindIndexBuffer(commandBuffer, m_indexBuffer->GetBuffer(), 0, VK_INDEX_TYPE_UINT32);

		VkDescriptorSet descriptorSet = m_vulkanGraphicsPipeline->GetDescriptorSet(m_currentFrame);

		vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_vulkanGraphicsPipeline->GetPipelineLayout(),
			0, // first set
			1, // descriptorSetCount
			&descriptorSet, 0, nullptr);

		vkCmdDrawIndexed(commandBuffer, static_cast<uint32_t>(quadIndices.size()), 1, 0, 0, 0);

		vkCmdEndRenderPass(commandBuffer);

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
