#include "pch.h"
#include "VulkanRenderer2D.h"
#include "Engine/Platform/Vulkan/VulkanContext.h"
#include <Engine/Platform/Vulkan/VulkanGraphicsPipeline.h>
#include "Engine/AssetManager/AssetManager.h"
#include "Engine/Platform/Vulkan/VulkanUtils.h"
#include <Engine/Events/Public/CollisionEvents.h>


#include "Renderer.h"
#include <backends/imgui_impl_vulkan.h>
#include <Engine/Map/Grid/TileCollisionMask.h>
#include <algorithm>
#include <Engine/Core/Core.h>
#include <utility>
#include <Engine/Map/TextureStreaming/TextureStreamingSystem.h>
#include "Engine/Core/Application.h"
#include <Engine/Math/HashUtils.h>

namespace Engine {

	//VulkanRenderer2D::SceneData* VulkanRenderer2D::m_sceneData = new SceneData();
	Engine::VulkanRenderer2DData Engine::VulkanRenderer2D::s_VulkanData;
	Engine::VulkanBindlessRenderer2DData Engine::VulkanRenderer2D::s_VulkanBindlessData;
	Engine::VulkanRenderer2DProjectileData Engine::VulkanRenderer2D::s_VulkanProjectileData;
	Ref<VulkanBindlessDescriptorSetRenderer> VulkanRenderer2D::s_bindlessDescitproSet;
	CollisionData Engine::VulkanRenderer2D::s_CollisionData;

	EffectPushConstants VulkanRenderer2D::s_effectPushConstants{
		/*textureOrigin*/ {0.0f, 0.0f},
		/*pixelSize*/      1.0f,
		/*textureIndex*/   0u,

		/*defaultTimer*/  24u,
		/*glowStrength*/  0.65f,
		/*maxTimer*/      64u,
		/*_pad0*/         0u
	};



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
			m_vulkanGraphicsPipelines->UpdateCameraUniformBuffer(i, m_camera->GetViewProjectionMatrix());
		}

		s_VulkanData.LineVertexBufferBase = new VulkanLineVertex[VulkanRenderer2DData::MaxLineVertices];

		s_VulkanData.LineStagingBuffer = std::make_shared<VulkanBuffer>(
			m_device,
			m_vulkanContext->GetDeviceManager().GetPhysicalDevice(),
			sizeof(VulkanLineVertex) * VulkanRenderer2DData::MaxLineVertices,
			VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
		);

		s_VulkanData.LineStagingBuffer->SetData(s_VulkanData.LineVertexBufferBase, sizeof(VulkanLineVertex) * VulkanRenderer2DData::MaxLineVertices);

		s_VulkanData.LineVertexBuffer = std::make_shared<VulkanBuffer>(
			m_device,
			m_vulkanContext->GetDeviceManager().GetPhysicalDevice(),
			sizeof(VulkanLineVertex) * VulkanRenderer2DData::MaxLineVertices,
			VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
		);
		s_VulkanData.LineVertexBuffer->SetData(s_VulkanData.LineVertexBufferBase, sizeof(VulkanLineVertex) * VulkanRenderer2DData::MaxLineVertices);

		s_VulkanData.LineVertexBufferPtr = s_VulkanData.LineVertexBufferBase;
		


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

		
		
		s_VulkanData.TextureSlots[0] = s_VulkanData.WhiteTexture;
		// Clone "logo" texture to slot 1 after loading it once
		// Load original textures (not inserted into slots)
		//AssetManager::AddTexture("logo", Engine::AssetManager::GetAssetPath("textures/ee_logo.png").string(), false);
		
		AssetManager::AddTexture("chess", Engine::AssetManager::GetAssetPath("textures/chess_board.png").string(), false);
		AssetManager::AddPixelTexture("pixel", Engine::AssetManager::GetAssetPath("textures/pixel_texture1.png").string());
		AssetManager::AddTexture("Idle_gun_000", Engine::AssetManager::GetAssetPath("textures/Idle_gun_000.png").string(), false);
		AssetManager::AddTexture("bullet", Engine::AssetManager::GetAssetPath("textures/Fire_small_asset.png").string(), false);
		AssetManager::AddTexture("zombie1_walk_000", Engine::AssetManager::GetAssetPath("textures/zombie1_walk_000.png").string(), false);
		AssetManager::AddTexture("wall_0019", Engine::AssetManager::GetAssetPath("textures/wall_0019.png").string(), false);
		AssetManager::AddTexture("zombie_walk_000", Engine::AssetManager::GetAssetPath("textures/zombie_walk_000.png").string(), false);
		AssetManager::AddTexture("objects_plant", Engine::AssetManager::GetAssetPath("textures/objects_plant.png").string(), false);
		AssetManager::AddTexture("car_0001", Engine::AssetManager::GetAssetPath("textures/car_0001.png").string(), false);
		AssetManager::AddTexture("house", Engine::AssetManager::GetAssetPath("textures/house.png").string(), false);

		
		m_dummyTexture = std::make_shared<VulkanTexture>(1, 1, VK_FORMAT_R8G8B8A8_UINT);
		m_dummyTexture->SetCheckCollision(false);

		// Fill the rest of the slots with pixel texture
		for (uint32_t i = s_VulkanData.TextureSlotIndex; i < s_VulkanData.MaxTextureSlots; i++)
		{
			s_VulkanData.TextureSlots[i] = s_VulkanData.WhiteTexture;
			s_VulkanData.TextureSlotIndex++;
		}

		for (uint32_t i = s_VulkanData.GridSlotIndex; i < s_VulkanData.GridSize; i++)
		{
			s_VulkanData.GridTextureSlots[i] = s_VulkanData.WhiteTexture;
			s_VulkanData.propertiesTextureSlots[i] = m_dummyTexture;
			s_VulkanData.VisualEffectsTextureSlots[i] = s_VulkanData.WhiteTexture;

			s_VulkanData.GridSlotIndex++;
		}
		

		for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
		{
			m_vulkanGraphicsPipelines->UpdateGameDrawAndVisualImagesDescriptorSets(i, s_VulkanData.TextureSlots, s_VulkanData.VisualEffectsTextureSlots);
		}
		s_VulkanData.TextureSlotIndex = CHUNK_GRID_SIZE;
		s_VulkanData.GridSlotIndex = 0;


		// this is for rendering game in editor viewport
		CreateImGuiTextureDescriptors();




		// *********** PROJECTILES **************
		s_VulkanProjectileData.QuadVertexBufferBase = new VulkanProjectileVertex[VulkanRenderer2DProjectileData::MaxProjectiles];
		s_VulkanProjectileData.QuadVertexBufferPtr = s_VulkanProjectileData.QuadVertexBufferBase;


	
		s_VulkanProjectileData.QuadVertexBuffer = std::make_shared<VulkanVertexBuffer>(
			reinterpret_cast<float*>(s_VulkanProjectileData.QuadVertexBufferBase),
			VulkanRenderer2DProjectileData::MaxProjectiles * sizeof(VulkanProjectileVertex)
		);

		std::vector<uint32_t> projectileIndices;
		projectileIndices.reserve(VulkanRenderer2DProjectileData::MaxProjectiles * 6);

		offset = 0;
		for (uint32_t i = 0; i < VulkanRenderer2DProjectileData::MaxProjectiles; i++) {
			projectileIndices.push_back(offset + 0);
			projectileIndices.push_back(offset + 1);
			projectileIndices.push_back(offset + 2);
			projectileIndices.push_back(offset + 2);
			projectileIndices.push_back(offset + 3);
			projectileIndices.push_back(offset + 0);
			offset += 4;
		}
		s_VulkanProjectileData.QuadIndexBuffer = std::make_shared<VulkanIndexBuffer>(
			projectileIndices.data(),
			projectileIndices.size()
		);

		//s_VulkanProjectileData.QuadIndexCount = static_cast<uint32_t>(projectileIndices.size());

		s_VulkanProjectileData.WhiteTexture = std::make_shared<VulkanTexture>(AssetManager::GetAssetPath("textures/white_texture.png").string());
		
		s_VulkanProjectileData.TextureSlots[0] = s_VulkanProjectileData.WhiteTexture;

		s_VulkanProjectileData.QuadVertexPositions[0] = { -0.5f, -0.5f, 0.0f };
		s_VulkanProjectileData.QuadVertexPositions[1] = { 0.5f, -0.5f, 0.0f };
		s_VulkanProjectileData.QuadVertexPositions[2] = { 0.5f,  0.5f, 0.0f };
		s_VulkanProjectileData.QuadVertexPositions[3] = { -0.5f,  0.5f, 0.0f };

		// You can optionally update descriptors here if needed
		s_VulkanProjectileData.TextureSlotIndex = 0;
		s_VulkanProjectileData.TextureSlots[s_VulkanProjectileData.TextureSlotIndex++] = AssetManager::GetTexture("Idle_gun_000");
		s_VulkanProjectileData.TextureSlots[s_VulkanProjectileData.TextureSlotIndex++] = AssetManager::GetTexture("bullet");
		
		for (uint32_t i = s_VulkanProjectileData.TextureSlotIndex; i < s_VulkanProjectileData.MaxProjectiles; i++)
		{
			s_VulkanProjectileData.TextureSlots[i] = s_VulkanProjectileData.WhiteTexture;

			s_VulkanProjectileData.TextureSlotIndex++;
		}

		for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
		{
			m_vulkanGraphicsPipelines->UpdateProjectileDescriptorSets(i, s_VulkanProjectileData.TextureSlots);
		
			
		}

	

		

		for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
		{
			m_vulkanGraphicsPipelines->UpdatePlayerCollisionDescriptorSet(i, s_VulkanData.propertiesTextureSlots);
		}



		uint32_t tilesPerRow = CHUNK_SIZE * CHUNK_GRID_WIDTH * GRID_SUBDIVISIONS;
		uint32_t totalTiles = tilesPerRow * tilesPerRow;
		Engine::TileBlockedMaskCPU::CachedGPUMask.resize(totalTiles);


		s_effectPushConstants = VulkanUtils::MakeDefaultEffectsState();


		s_bindlessDescitproSet = std::make_shared<VulkanBindlessDescriptorSetRenderer>(m_device, false);


		s_VulkanBindlessData.m_slotOriginWorld.resize(MAX_RESIDENT_LAYERS);
	}

	void VulkanRenderer2D::BeginFrame(uint32_t currentFrame)
	{


		EE_PROFILE_FUNCTION();
		// clear old textures when GPU is done with them
		// Sync
	
		vkWaitForFences(m_device, 1, &m_inFlightFences[currentFrame], VK_TRUE, UINT64_MAX);
		vkResetFences(m_device, 1, &m_inFlightFences[currentFrame]);

		StartBatch();
		
		
		

	}

	void VulkanRenderer2D::EndFrame(uint32_t currentFrame)
	{
		EE_PROFILE_FUNCTION();
		s_bindlessDescitproSet->UpdateEffectImageDescriptorSets(currentFrame, s_VulkanData.VisualEffectsTextureSlots);

		CalculateCollisionFrame(currentFrame);
		s_VulkanData.CurrentFrame = currentFrame;

		VkCommandBuffer cmd = m_commandBuffers[currentFrame];

		m_vulkanGraphicsPipelines->UpdateCameraUniformBuffer(currentFrame, s_VulkanData.CameraBuffer.ViewProjection);

		// Acquire. Max current frame is 2 and max swapchain images is 3.
		// set in Renderer.h 	const int MAX_FRAMES_IN_FLIGHT = 2;

		VkResult result = vkAcquireNextImageKHR(m_device, m_swapchain, UINT64_MAX, m_imageAvailableSemaphores[currentFrame], VK_NULL_HANDLE, &m_imageIndex);

		if (result == VK_ERROR_OUT_OF_DATE_KHR)
		{
			m_vulkanContext->GetVulkanSwapchain().RecreateSwapchain();
			CreateSyncObjects();
			m_swapchain = m_vulkanContext->GetVulkanSwapchain().GetSwapchain();
			m_swapchainExtent = m_vulkanContext->GetVulkanSwapchain().GetSwapchainExtent();
			return;
		}
		else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR)
		{
			EE_CORE_ASSERT(false, "Failed to acquire swapchain image!");
		}


		/*
		// Record Game Pass
		vkResetCommandBuffer(cmd, 0);

		VkCommandBufferBeginInfo beginInfo{};
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

		vkBeginCommandBuffer(cmd, &beginInfo);
		*/

		s_bindlessDescitproSet->SetCurrentFrameIndex(currentFrame);
		ConsumeDestructibleQueue(cmd, currentFrame);


		//move somewhere
		s_CollisionData.EntitySlotIndex = 0;
		s_VulkanData.TextureSlotIndex = 0;
		s_VulkanData.GridSlotIndex = 0;
		s_VulkanData.VisualTextureSlotIndex = 0;
		s_VulkanProjectileData.TextureSlotIndex = 1;
		s_VulkanData.TextureToSlotMap.clear();
		s_VulkanBindlessData.submitQueues[currentFrame].clear();


		// --- Begin render pass ---
		VkRenderPassBeginInfo renderPassInfo{};
		renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		renderPassInfo.renderPass = m_vulkanContext->GetGameRenderPass();
		renderPassInfo.framebuffer = m_vulkanContext->GetVulkanSwapchain().GetGameFramebuffer(m_imageIndex);
		renderPassInfo.renderArea = { {0, 0}, m_vulkanContext->GetVulkanSwapchain().GetSwapchainExtent() };

		// Clear color for the color attachment
		VkClearValue clearColor = { {0.2f, 0.2f, 0.35f, 1.0f} };
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



		m_firstIndex = 0;
		m_vertexOffset = 0;
		uint32_t dataSize = (uint32_t)((uint8_t*)s_VulkanData.QuadVertexBufferPtr - (uint8_t*)s_VulkanData.QuadVertexBufferBase);
		if (dataSize > 0)
		{
			EE_PROFILE_SCOPE("Flush Quad");
			s_VulkanData.QuadVertexBuffer->SetData(s_VulkanData.QuadVertexBufferBase, dataSize);
		}

		if (s_VulkanData.LineVertexCount > 0)
		{
			EE_PROFILE_SCOPE("Flush line");

			s_VulkanData.LineVertexBuffer->SetData(s_VulkanData.LineVertexBufferBase, s_VulkanData.LineVertexCount * sizeof(VulkanLineVertex));
		}

		dataSize = (uint32_t)((uint8_t*)s_VulkanProjectileData.QuadVertexBufferPtr - (uint8_t*)s_VulkanProjectileData.QuadVertexBufferBase);
		if (dataSize > 0)
		{
			//auto* v = s_VulkanProjectileData.QuadVertexBufferBase;
			//EE_CORE_INFO("Projectile Vertex[0] Pos: ({}, {}, {})", v[0].Position.x, v[0].Position.y, v[0].Position.z);
			//EE_CORE_INFO("TexCoord: ({}, {}), TexIndex: {}", v[0].TexCoord.x, v[0].TexCoord.y, v[0].TexIndex);
			EE_PROFILE_SCOPE("Flush projectile");
			s_VulkanProjectileData.QuadVertexBuffer->SetData(s_VulkanProjectileData.QuadVertexBufferBase, dataSize);
			

		}
		m_vulkanGraphicsPipelines->UpdateGameDrawAndVisualImagesDescriptorSets(currentFrame, s_VulkanData.TextureSlots, s_VulkanData.VisualEffectsTextureSlots);
		m_vulkanGraphicsPipelines->UpdateProjectileDescriptorSets(currentFrame, s_VulkanProjectileData.TextureSlots);


		
		Draw();
		

		// End RecordGameDrawCommands render pass
		vkCmdEndRenderPass(cmd);

		RecordPresentDrawCommands(cmd, m_imageIndex, currentFrame);
		RecordEditorDrawCommands(cmd, m_imageIndex, currentFrame);

		vkEndCommandBuffer(cmd);

		// Submit
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

		// Present
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
			CreateSyncObjects();
			m_swapchain = m_vulkanContext->GetVulkanSwapchain().GetSwapchain();
			m_swapchainExtent = m_vulkanContext->GetVulkanSwapchain().GetSwapchainExtent();
			return;
		}
		else if (presentResult != VK_SUCCESS)
		{
			EE_CORE_ASSERT(false, "Failed to present swapchain image!");
		}

	


		


		


	}

	void VulkanRenderer2D::CalculateCollisionFrame(uint32_t currentFrame)
	{
		EE_PROFILE_FUNCTION();

		/*
		void* data = nullptr;
		VkDeviceSize size = sizeof(CollisionResultBuffer);
		vkMapMemory(m_device, m_vulkanGraphicsPipelines->GetGPUCollisionMemory(), 0, size, 0, &data);
		std::memset(data, 0xFFFFFFFFu, size);
		vkUnmapMemory(m_device, m_vulkanGraphicsPipelines->GetGPUCollisionMemory());
		
		*/
		


		VkCommandBuffer cmd = m_commandBuffers[currentFrame];
		// Clear counter
		

		vkResetCommandBuffer(cmd, 0);

		VkCommandBufferBeginInfo beginInfo{};
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

		vkBeginCommandBuffer(cmd, &beginInfo);

		// projectiles
		m_vulkanGraphicsPipelines->UpdateCollisionUniformBuffer(currentFrame, s_CollisionData.CollisionEntities);
		RecordComputeCommandBuffer(cmd, currentFrame);


		// using grid at the moment
		//m_vulkanGraphicsPipelines->UpdatePLayerCollisionUniformBuffer(currentFrame, s_CollisionData.playerEntities);
		//RecordPlayerCommandBuffer(cmd, m_imageIndex, currentFrame);
		
		/*
		*/
		{
			// barrier so that effects can see the results of compute
			VkBufferMemoryBarrier b{};
			b.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
			b.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
			b.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
			b.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			b.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			b.buffer = m_vulkanGraphicsPipelines->GetGPUCollisionResultBuffer(); 
			b.offset = 0;
			b.size = VK_WHOLE_SIZE;

			vkCmdPipelineBarrier(cmd,
				VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
				VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
				0, 0, nullptr, 1, &b, 0, nullptr);


			RecordEffectComputeCommandBuffer(cmd, currentFrame);

		}

		
		
		/*
		vkEndCommandBuffer(cmd);

		VkSubmitInfo submitInfo{};
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &cmd;

		if (vkQueueSubmit(m_vulkanContext->GetGraphicsQueue(), 1, &submitInfo, m_inFlightFences[currentFrame]) != VK_SUCCESS)
		{
			EE_CORE_ASSERT(false, "Failed to submit compute command buffer!");
		}

		{
			EE_PROFILE_SCOPE("FENCES");
			// Wait for compute to finish before reading buffer (optional, or use fence wait elsewhere)
			vkWaitForFences(m_device, 1, &m_inFlightFences[currentFrame], VK_TRUE, UINT64_MAX);
			vkResetFences(m_device, 1, &m_inFlightFences[currentFrame]);

		}
		*/


		{
			
			EE_PROFILE_SCOPE("collision results");

			//ReadPlayerCollisionBuffer();

			// move this to own method
			// Read back collision results
			
			 
			{
				// reset player collision. remove?
				void* data = nullptr;
				VkDeviceSize size = sizeof(CollisionResultBuffer);

				vkMapMemory(m_device, m_vulkanGraphicsPipelines->GetPlayerCollisionMemory(), 0, size, 0, &data);
				std::memset(data, 0, size);
				vkUnmapMemory(m_device, m_vulkanGraphicsPipelines->GetPlayerCollisionMemory());
			}

		}


		{
			EE_PROFILE_SCOPE("blocked tiles buffer");
			uint32_t tilesPerRow = CHUNK_SIZE * CHUNK_GRID_WIDTH * GRID_SUBDIVISIONS;
			uint32_t totalTiles = tilesPerRow * tilesPerRow;

			std::vector<uint32_t> gpuBlockedTileMask(totalTiles);
			ReadBlockedTileMask(gpuBlockedTileMask, totalTiles);
			
			
			/*
			for (size_t i = 0; i < totalTiles; i++)
			{
				if (gpuBlockedTileMask[i] == 2u)
				{
					EE_CORE_INFO("destroyed tile {}", i);
				}
			}
			*/
			
			
			

			Engine::TileBlockedMaskCPU::CachedGPUMask = std::move(gpuBlockedTileMask);

		}


	}

	/*
	void VulkanRenderer2D::ReadPlayerCollisionBuffer()
	{
		CollisionResultBuffer result = {};
		void* data = nullptr;
		vkMapMemory(m_device, m_vulkanGraphicsPipelines->GetPlayerCollisionMemory(), 0, sizeof(result), 0, &data);
		memcpy(&result, data, sizeof(result));
		vkUnmapMemory(m_device, m_vulkanGraphicsPipelines->GetPlayerCollisionMemory());

		if (result.collisionCount > 0)
		{
			CollisionResultsCPU::PlayerCollisions.clear();
			CollisionResultsCPU::PlayerCollisions.reserve(MAX_COLLISION_RESULTS);

			const uint32_t cap = MAX_COLLISION_RESULTS;

			for (uint32_t i = 0; i < cap; ++i)
			{
				const auto& r = result.results[i];
				if (r.collisionDetected == 0)
					continue;

				Collision coll{};
				// If your CollisionResult has Low/High parts, reconstruct here;
				// otherwise keep using r.GetProjectileID() if that helper exists.
				// coll.ProjectileID = (uint64_t(r.hitProjectileID_High) << 32) | uint64_t(r.hitProjectileID_Low);
				coll.EntityID = r.GetProjectileID();  // if your CPU-side struct provides it
				coll.HitPosition = r.CollisionPosition;
				coll.Health = r.Health;        // note: in shader it's HealthAfter
				CollisionResultsCPU::PlayerCollisions.push_back(coll);
			}



		}

	}

	*/
	void VulkanRenderer2D::ReadBlockedTileMask(std::vector<uint32_t>& outDestroyedMask, uint32_t count)
	{
		void* data;
		vkMapMemory(m_device, m_vulkanGraphicsPipelines->GetBlockedTileMaskMemory(), 0, sizeof(uint32_t) * count, 0, &data);

		// Copy data from GPU buffer memory to CPU vector
		memcpy(outDestroyedMask.data(), data, sizeof(uint32_t) * count);

		vkUnmapMemory(m_device, m_vulkanGraphicsPipelines->GetBlockedTileMaskMemory());
	}


	void VulkanRenderer2D::DeviceWaitIdle()
	{
		vkDeviceWaitIdle(m_device);
	}

	void VulkanRenderer2D::StartBatch()
	{
		s_VulkanData.QuadVertexBufferPtr = s_VulkanData.QuadVertexBufferBase;
		s_VulkanData.QuadIndexCount = 0;
		

		s_VulkanData.LineVertexBufferPtr = s_VulkanData.LineVertexBufferBase;
		s_VulkanData.LineVertexCount = 0;

		s_VulkanProjectileData.QuadVertexBufferPtr = s_VulkanProjectileData.QuadVertexBufferBase;
		s_VulkanProjectileData.QuadIndexCount = 0;


		// collisions
		// do this at end?
		

	}

	/*
	void VulkanRenderer2D::FlushLines()
	{
		if (s_VulkanData.LineVertexCount == 0)
			return;

		// Copy CPU-side data to staging buffer
		void* data = s_VulkanData.LineStagingBuffer->Map();
		memcpy(data, s_VulkanData.LineVertexBufferBase, s_VulkanData.LineVertexCount * sizeof(VulkanLineVertex));
		s_VulkanData.LineStagingBuffer->Unmap();

		// Upload to GPU
		VulkanUtils::CopyBuffer(
			s_VulkanData.LineStagingBuffer->GetBuffer(),
			s_VulkanData.LineVertexBuffer->GetBuffer(),
			s_VulkanData.LineVertexCount * sizeof(VulkanLineVertex)
		);
	}
	*/


	void VulkanRenderer2D::NextBatch()
	{
		StartBatch();
		//Draw();
	}

	void VulkanRenderer2D::Draw()
	{
		Renderer::DrawFrame();
	}

	void VulkanRenderer2D::DrawFrame(uint32_t currentFrame)
	{
		EE_PROFILE_FUNCTION();

		//this can be called multiple times per frame
		VkCommandBuffer cmd = m_commandBuffers[currentFrame];


		
		RecordGameDrawCommands(cmd, m_imageIndex, currentFrame);
		RecordProjectileDrawCommands(cmd, currentFrame, currentFrame);
		RecordLineCommanedBuffer(cmd, m_imageIndex, currentFrame);


		s_bindlessDescitproSet->RecordTiles(cmd, currentFrame,
			s_VulkanData.CameraBuffer.ViewProjection, m_vulkanContext->GetVulkanSwapchain().GetSwapchainExtent());

	}

	void VulkanRenderer2D::RecordGameDrawCommands(VkCommandBuffer commandBuffer, uint32_t imageIndex, uint32_t currentFrame)
	{
		EE_PROFILE_FUNCTION();


		VkBuffer vertexBuffers[] = { s_VulkanData.QuadVertexBuffer->GetBuffer() };

		VkDeviceSize offsets[] = { 0 };
		vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);

		//vkCmdBindVertexBuffers(commandBuffer, 2, 1, BulletBuffers, &offsets[1]);

		vkCmdBindIndexBuffer(commandBuffer, s_VulkanData.QuadIndexBuffer->GetBuffer(), 0, VK_INDEX_TYPE_UINT32);

		VkDescriptorSet descriptorSet = m_vulkanGraphicsPipelines->GetGameDescriptorSet(currentFrame);
		vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_vulkanGraphicsPipelines->GetGamePipelineLayout(), 1, 1, &descriptorSet, 0, nullptr);

		descriptorSet = m_vulkanGraphicsPipelines->GetCameraDescriptorSet(currentFrame);
		vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_vulkanGraphicsPipelines->GetGamePipelineLayout(), 0, 1, &descriptorSet, 0, nullptr);

		vkCmdDrawIndexed(commandBuffer, s_VulkanData.QuadIndexCount, 1, 0, 0, 0);

		s_VulkanData.Stats.DrawCalls++;

	}

	void VulkanRenderer2D::RecordProjectileDrawCommands(VkCommandBuffer cmd, uint32_t frameIndex, uint32_t currentFrame)
	{
		if (s_VulkanProjectileData.QuadIndexCount == 0)
			return;

		// Bind pipeline
		vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_vulkanGraphicsPipelines->GetProjectilePipeline());

		// Bind descriptor set (for textures)
		VkDescriptorSet descriptorSets[] = {
			m_vulkanGraphicsPipelines->GetCameraDescriptorSet(currentFrame),     // set = 0
			m_vulkanGraphicsPipelines->GetProjectileDescriptorSet(frameIndex)    // set = 1
				};

		vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_vulkanGraphicsPipelines->GetProjectilePipelineLayout(), 0, 2, descriptorSets, 0, nullptr);

		// Update GPU buffer if needed

		// Bind buffers
		VkBuffer vertexBuffers[] = { s_VulkanProjectileData.QuadVertexBuffer->GetBuffer() };
		VkDeviceSize offsets[] = { 0 };
		vkCmdBindVertexBuffers(cmd, 0, 1, vertexBuffers, offsets);
		vkCmdBindIndexBuffer(cmd, s_VulkanProjectileData.QuadIndexBuffer->GetBuffer(), 0, VK_INDEX_TYPE_UINT32);

		// Draw
		vkCmdDrawIndexed(cmd, s_VulkanProjectileData.QuadIndexCount, 1, 0, 0, 0);

		s_VulkanData.Stats.DrawCalls++;

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

	/*
	void VulkanRenderer2D::RecordComputeCommanedBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex, uint32_t currentFrame)
	{

		
		EE_PROFILE_FUNCTION();


		// only reset when needed?
		uint32_t tilesPerRow = CHUNK_SIZE * CHUNK_GRID_WIDTH * GRID_SUBDIVISIONS;
		uint32_t totalTiles = tilesPerRow * tilesPerRow;
		void* data;
		vkMapMemory(m_device, m_vulkanGraphicsPipelines->GetBlockedTileMaskMemory(), 0, totalTiles * sizeof(uint32_t), 0, &data);

		std::fill_n(static_cast<uint32_t*>(data), totalTiles, 0u);
		vkUnmapMemory(m_device, m_vulkanGraphicsPipelines->GetBlockedTileMaskMemory());



		// Use current texture slots for both read and write (in-place compute)
		std::array<Ref<VulkanTexture>, CHUNK_GRID_SIZE>& computeTextures = s_VulkanData.GridTextureSlots;
		std::array<Ref<VulkanTexture>, CHUNK_GRID_SIZE>& propertiesTextures = s_VulkanData.propertiesTextureSlots;

		// Update descriptor set with same textures for read/write
		m_vulkanGraphicsPipelines->UpdateComputeDescriptorSet(currentFrame,	computeTextures, propertiesTextures);

		glm::ivec2 minOrigin = { std::numeric_limits<int>::max(), std::numeric_limits<int>::max() };
		// Transition textures to GENERAL layout
		for (size_t i = 0; i < CHUNK_GRID_SIZE; i++)
		{
			VulkanTexture& tex = *computeTextures[i];

			if (tex.GetCurrentLayout() != VK_IMAGE_LAYOUT_GENERAL)
			{
				TransitionImageLayout(commandBuffer,
					tex.GetImage(),
					tex.GetCurrentLayout(),
					VK_IMAGE_LAYOUT_GENERAL);

				tex.SetCurrentLayout(VK_IMAGE_LAYOUT_GENERAL);

				
					glm::ivec2 texOrigin = tex.GetTextureOrigin();
					glm::ivec2 tileCoord = glm::floor(glm::vec2(texOrigin) / float(TILE_PIXEL_WIDTH));

					//EE_CORE_INFO("Chunk Index {}: CheckCollision = true, texOrigin = ({}, {}), tileCoord = ({}, {})",
					//	i, texOrigin.x, texOrigin.y, tileCoord.x, tileCoord.y);

					minOrigin.x = std::min(minOrigin.x, tileCoord.x);
					minOrigin.y = std::min(minOrigin.y, tileCoord.y);
				
				

			}
		}


		for (size_t i = 0; i < CHUNK_GRID_SIZE; i++)
		{
			VulkanTexture& healthTex = *propertiesTextures[i];
			if (healthTex.GetCurrentLayout() != VK_IMAGE_LAYOUT_GENERAL)
			{
				TransitionImageLayout(commandBuffer,
					healthTex.GetImage(),
					healthTex.GetCurrentLayout(),
					VK_IMAGE_LAYOUT_GENERAL);

				healthTex.SetCurrentLayout(VK_IMAGE_LAYOUT_GENERAL);
			}
		}

		// Bind pipeline and descriptor set
		vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, m_vulkanGraphicsPipelines->GetComputePipeline());

		VkDescriptorSet descriptorSet = m_vulkanGraphicsPipelines->GetComputeDescriptorSet(currentFrame);
		vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE,
			m_vulkanGraphicsPipelines->GetComputePipelineLayout(), 0, 1, &descriptorSet, 0, nullptr);


		// Dispatch compute work per active texture
		for (size_t i = 0; i < CHUNK_GRID_SIZE; i++)
		{
			VulkanTexture& tex = *computeTextures[i];

			if (!tex.GetCheckCollision())
			{
				//Skip dummy textures
				continue;
			}

			PushConstants pushconstant{};
			pushconstant.TextureOrigin = tex.GetTextureOrigin();
			//EE_CORE_INFO("texture origin {} | {}, for index {}", tex.GetTextureOrigin().x, tex.GetTextureOrigin().y, i);
			//EE_CORE_INFO("texture minOrigin {} | {}, for index {}", minOrigin.x, minOrigin.y, i);

			pushconstant.PixelSize = tex.GetPixelSize();
			pushconstant.textureIndex = static_cast<uint32_t>(i);
			pushconstant.NumProjectiles = s_CollisionData.EntitySlotIndex;
			pushconstant.ChunkSize = TILE_PIXEL_WIDTH * CHUNK_SIZE; // unused for now
			pushconstant.TileSize = TILE_PIXEL_WIDTH;
			//pushconstant.mode = 0;

			//debug
			pushconstant.mode = 0;
			//pushconstant.mode |= 1u;   // show start/end/trail
			//pushconstant.mode |= 8u;   // paint reasons at overlap pixel

			// If you still see "nothing", try:
			//pushconstant.mode |= 2u;   // ignore Z gate   if you get hits now, Z was the problem
			// or:
			//pushconstant.mode |= 4u;   // force solid      if you get hits now, Properties.R was 0
			// optionally:
			//pushconstant.mode |= 16u;  // ignore claim     see if something earlier was claiming
			//pushconstant.mode |= 32u;  // ignore claim     see if something earlier was claiming
			//pushconstant.mode |= 128u;  
			//pushconstant.mode |= 256u;  
			//pushconstant.mode |= 512u;

			pushconstant.MinTileCoords = minOrigin * (int)CHUNK_SIZE;

			//EE_CORE_INFO("EntitySlotIndex {}",s_CollisionData.EntitySlotIndex);
			
			vkCmdPushConstants(commandBuffer,
				m_vulkanGraphicsPipelines->GetComputePipelineLayout(),
				VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(PushConstants), &pushconstant);

			uint32_t groupSizeX = 16;
			uint32_t groupSizeY = 16;

			uint32_t dispatchX = (tex.GetWidth() + groupSizeX - 1) / groupSizeX;
			uint32_t dispatchY = (tex.GetHeight() + groupSizeY - 1) / groupSizeY;

			vkCmdDispatch(commandBuffer, dispatchX, dispatchY, 1);
		}

		// Transition all textures back to SHADER_READ_ONLY_OPTIMAL
		for (size_t i = 0; i < CHUNK_GRID_SIZE; i++)
		{
			VulkanTexture& tex = *computeTextures[i];

			if (tex.GetCurrentLayout() != VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
			{
				TransitionImageLayout(commandBuffer,
					tex.GetImage(),
					tex.GetCurrentLayout(),
					VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

				tex.SetCurrentLayout(VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
			}
		}
		
		// Transition HealthTextures back to SHADER_READ_ONLY_OPTIMAL if needed
		for (size_t i = 0; i < MAX_TEXTURES; i++)
		{
			VulkanTexture& healthTex = *HealthTextures[i];
			if (healthTex.GetCurrentLayout() != VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
			{
				TransitionImageLayout(commandBuffer,
					healthTex.GetImage(),
					healthTex.GetCurrentLayout(),
					VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

				healthTex.SetCurrentLayout(VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
			}
		
		}
	}


	*/

	static inline void BarrierLayer(VkCommandBuffer cmd, VkImage img, uint32_t layer,
		VkImageLayout oldL, VkImageLayout newL,
		VkPipelineStageFlags srcStage, VkPipelineStageFlags dstStage,
		VkAccessFlags srcAccess, VkAccessFlags dstAccess)
	{
		VkImageMemoryBarrier b{};
		b.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		b.srcAccessMask = srcAccess;
		b.dstAccessMask = dstAccess;
		b.oldLayout = oldL;
		b.newLayout = newL;
		b.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		b.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		b.image = img;
		b.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		b.subresourceRange.baseMipLevel = 0;
		b.subresourceRange.levelCount = 1;
		b.subresourceRange.baseArrayLayer = layer;
		b.subresourceRange.layerCount = 1;

		vkCmdPipelineBarrier(cmd, srcStage, dstStage, 0,
			0, nullptr, 0, nullptr, 1, &b);
	}


	void VulkanRenderer2D::RecordComputeCommandBuffer(VkCommandBuffer cmd, uint32_t frameIndex)
	{

		// 0) Bind compute pipeline + its bindless compute set
		vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_COMPUTE,
			s_bindlessDescitproSet->GetComputePipeline());
		VkDescriptorSet set0 = s_bindlessDescitproSet->GetComputeDescriptorSetFrame(frameIndex);
		vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_COMPUTE,
			s_bindlessDescitproSet->GetComputePipelineLayout(),
			0, 1, &set0, 0, nullptr);

		// 1) Bind buffers (results/projectiles/mask) for this frame
		VkBuffer blockedMaskBuffer = m_vulkanGraphicsPipelines->GetBlockedTileMaskBuffer();
		uint32_t tilesPerRow = CHUNK_SIZE * CHUNK_GRID_WIDTH * GRID_SUBDIVISIONS;
		uint32_t tilesPerMask = tilesPerRow * tilesPerRow;
		VkDeviceSize blockedMaskBufferSize = sizeof(uint32_t) * tilesPerMask;

		VkBuffer collisionResultBuffer = m_vulkanGraphicsPipelines->GetGPUCollisionResultBuffer();
		VkDeviceSize collisionResultBufferSize = sizeof(CollisionResultBuffer);

		VkBuffer projectileBuffer = m_vulkanGraphicsPipelines->GetBulletUniformBuffer(frameIndex).GetBuffer();
		VkDeviceSize projectileBufferSize = m_vulkanGraphicsPipelines->GetBulletUniformBuffer(frameIndex).size;

		s_bindlessDescitproSet->ComputeBindBuffers(frameIndex,
			collisionResultBuffer, collisionResultBufferSize,
			projectileBuffer, projectileBufferSize,
			blockedMaskBuffer, blockedMaskBufferSize);

		// 2) Dispatch once per slot we rendered this frame
		VkImage colorArray = s_bindlessDescitproSet->GetColorImageArray();
		VkImage propsArray = s_bindlessDescitproSet->GetPropsArrayImage(); // stays GENERAL; no barrier needed here

		// Dedup slots (m_tileToSlot is uid -> slot)
		std::unordered_set<uint32_t> uniqueSlots;
		uniqueSlots.reserve(s_bindlessDescitproSet->GetTileToSlotMap().size());
		for (const std::pair<const uint64_t, uint32_t>& kv : s_bindlessDescitproSet->GetTileToSlotMap())
			uniqueSlots.insert(kv.second);

		const int tileW = TILE_PIXEL_WIDTH;
		const int tileH = TILE_PIXEL_HEIGHT;
		const float pixelSizeWorld =
			(tileW > 0) ? (float)TILE_SIZE / float(tileW) : 1.0f;

		static constexpr uint32_t kLocalX = 16;
		static constexpr uint32_t kLocalY = 16;
		auto CeilDiv = [](uint32_t n, uint32_t d) { return (n + d - 1) / d; };

		for (uint32_t slot : uniqueSlots)
		{

			// ---- Build push constants ----
			ComputePC pc{};
			pc.TextureIndex = slot;
			pc.TextureOriginWorld = s_VulkanBindlessData.m_slotOriginWorld[slot]; // TOP-LEFT in world
			pc.PixelSizeWorld = pixelSizeWorld;                                // world/px
			pc.NumProjectiles = s_CollisionData.EntitySlotIndex;
			pc.TileSizePixels = static_cast<uint32_t>(tileW);                  // tile width (e.g. 128)
		

			// ---- Transition color layer for compute (props array should already be GENERAL) ----
			BarrierLayer(cmd, colorArray, slot,
				VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_LAYOUT_GENERAL,
				VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
				VK_ACCESS_SHADER_READ_BIT, VK_ACCESS_SHADER_WRITE_BIT);



			// ---- Push & dispatch over the CONTENT area (no rectMin; shader uses contentMinPx+lid) ----
			vkCmdPushConstants(cmd,
				s_bindlessDescitproSet->GetComputePipelineLayout(),
				VK_SHADER_STAGE_COMPUTE_BIT,
				0, sizeof(ComputePC), &pc);


			
			const uint32_t gx = CeilDiv(TILE_PIXEL_WIDTH, kLocalX);
			const uint32_t gy = CeilDiv(TILE_PIXEL_HEIGHT, kLocalY);
			vkCmdDispatch(cmd, gx, gy, 1);

			/*
			*/
			VkMemoryBarrier mb{ VK_STRUCTURE_TYPE_MEMORY_BARRIER };
			mb.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
			mb.dstAccessMask = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT;

			vkCmdPipelineBarrier(cmd,
				VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
				VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
				0, 1, &mb, 0, nullptr, 0, nullptr);


			/*
			// ---- Back to read-only for the graphics pass ----
			BarrierLayer(cmd, colorArray, slot,
				VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
				VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
				VK_ACCESS_SHADER_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT);
			*/
		}


	}






	void VulkanRenderer2D::RecordPlayerCommandBuffer(VkCommandBuffer cmd, uint32_t imageIndex, uint32_t currentFrame)
	{
		EE_PROFILE_FUNCTION();

		// --- (Optional) If you use a separate results buffer for player collisions, clear it here ---
		// {
		//     void* data = nullptr;
		//     VkDeviceSize size = sizeof(CollisionResultBuffer);
		//     vkMapMemory(m_device, m_vulkanGraphicsPipelines->GetGPUCollisionMemory_Player(), 0, size, 0, &data);
		//     std::memset(data, 0, size);
		//     vkUnmapMemory(m_device, m_vulkanGraphicsPipelines->GetGPUCollisionMemory_Player());
		// }

		// Current grid slots (same arrays you already maintain)
		auto& healthTex = s_VulkanData.propertiesTextureSlots;   // we’ll only bind/use the center (index 4)

		// Center chunk index in 3x3
		constexpr uint32_t CENTER = 4;
		EE_CORE_ASSERT(healthTex[CENTER], "Center chunk health texture is null");

		// Transition center health image -> GENERAL
		{
			VulkanTexture& h = *healthTex[CENTER];
			if (h.GetCurrentLayout() != VK_IMAGE_LAYOUT_GENERAL) {
				TransitionImageLayout(cmd, h.GetImage(), h.GetCurrentLayout(), VK_IMAGE_LAYOUT_GENERAL);
				h.SetCurrentLayout(VK_IMAGE_LAYOUT_GENERAL);
			}
		}

		// Update player-collision descriptor set (binds only the center health image at binding 0)
		m_vulkanGraphicsPipelines->UpdatePlayerCollisionDescriptorSet(currentFrame, healthTex);
		VkDescriptorSet ds = m_vulkanGraphicsPipelines->GetPlayerCollisionComputeDescriptorSet(currentFrame);

		// Bind player-collision pipeline
		vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, m_vulkanGraphicsPipelines->GetPlayerCollisionComputePipeline());
		vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_COMPUTE,
			m_vulkanGraphicsPipelines->GetPlayerCollisionComputePipelineLayout(),
			0, 1, &ds, 0, nullptr);

		// Build push constants for the center chunk only
		// NOTE: reusing your PushConstants layout. If your player shader expects a different struct,
		//       change this to match.
		PlayerPC pc{}; // your original struct with TextureOrigin, PixelSize, TextureIndex, etc.

		{
			VulkanTexture& centerHealth= *healthTex[CENTER];  // for origin/pixel size
			pc.WindowOriginWorld = centerHealth.GetTextureOrigin();  // top-left in world-units
			pc.PixelSizeWorld = centerHealth.GetPixelSize();

			
			pc.NumPlayers = PLAYER_COUNT; 

			pc.ChunkSizePixels = TILE_PIXEL_WIDTH * CHUNK_SIZE;
			
		}

		vkCmdPushConstants(cmd,
			m_vulkanGraphicsPipelines->GetPlayerCollisionComputePipelineLayout(),
			VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(PlayerPC), &pc);

		// Dispatch only over the center chunk
		{
			const VulkanTexture& centerH = *healthTex[CENTER];
			const uint32_t w = centerH.GetWidth();
			const uint32_t h = centerH.GetHeight();

			const uint32_t groupSizeX = 16;
			const uint32_t groupSizeY = 16;

			const uint32_t dispatchX = (w + groupSizeX - 1) / groupSizeX;
			const uint32_t dispatchY = (h + groupSizeY - 1) / groupSizeY;

			vkCmdDispatch(cmd, dispatchX, dispatchY, 1);
		}

		// Ensure the player pass writes (results/health) are visible to CPU or subsequent passes
		{
			VkMemoryBarrier barrier{};
			barrier.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER;
			barrier.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
			barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT;

			vkCmdPipelineBarrier(cmd,
				VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
				VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
				0, 1, &barrier, 0, nullptr, 0, nullptr);
		}

		// pass samples it with a sampled image. If it remains a storage image for later compute, skip this.
		// {
		//     VulkanTexture& h = *healthTex[CENTER];
		//     if (h.GetCurrentLayout() != VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
		//         TransitionImageLayout(cmd, h.GetImage(), h.GetCurrentLayout(), VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
		//         h.SetCurrentLayout(VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
		//     }
		// }
	}


	// Same test you use in the shader
	static inline bool CircleIntersectsRect(glm::vec2 cW, float rW,
		glm::vec2 minW, glm::vec2 maxW)
	{
		const float nx = std::clamp(cW.x, minW.x, maxW.x);
		const float ny = std::clamp(cW.y, minW.y, maxW.y);
		const glm::vec2 d = cW - glm::vec2(nx, ny);
		return glm::dot(d, d) <= rW * rW + 1e-6f;
	}


	void VulkanRenderer2D::BuildAffectedTilesCPU(
		const std::vector<glm::vec2>& hitPositionsW,          // world hits this frame
		const std::vector<float>& radiiW,                 // same length as hits
		const std::vector<uint32_t>& damagesW,               // same length as hits
		const std::unordered_set<uint32_t>& candidateSlots,   // visible/active slots
		float pixelSizeWorld, int tileW, int tileH,
		std::vector<AffectedTile>& outTiles)
	{
		outTiles.clear();
		outTiles.reserve(candidateSlots.size());

		if (hitPositionsW.empty() || radiiW.size() != hitPositionsW.size() || damagesW.size() != hitPositionsW.size())
			return; // nothing to do or mismatched inputs

		for (uint32_t slot : candidateSlots)
		{
			// Tile AABB in world
			const glm::vec2 minW = s_VulkanBindlessData.m_slotOriginWorld[slot]; // top-left in world
			const glm::vec2 maxW = minW + glm::vec2(tileW, tileH) * pixelSizeWorld;

			uint32_t totalDamage = 0.0f;
			float maxRadius = 0.0f;
			bool  touched = false;

			// Accumulate all hits that touch this tile
			for (size_t i = 0; i < hitPositionsW.size(); ++i)
			{
				if (CircleIntersectsRect(hitPositionsW[i], radiiW[i], minW, maxW))
				{
					touched = true;
					totalDamage += damagesW[i];
					if (radiiW[i] > maxRadius) maxRadius = radiiW[i];
				}
			}

			if (touched)
			{
				outTiles.push_back(AffectedTile{
					slot = slot,
					totalDamage = totalDamage,
					maxRadius = maxRadius
					});
			}
		}
	}


	void VulkanRenderer2D::RecordEffectComputeCommandBuffer(VkCommandBuffer cmd, uint32_t frameIndex)
	{
		EE_PROFILE_FUNCTION();

		// Reset any effects
		{
			void* data = nullptr;
			vkMapMemory(m_device, m_vulkanGraphicsPipelines->GetEffectsBufferMemory(),
				0, sizeof(uint32_t), 0, &data);
			*reinterpret_cast<uint32_t*>(data) = 0u;
			vkUnmapMemory(m_device, m_vulkanGraphicsPipelines->GetEffectsBufferMemory());
		}

		if (s_bindlessDescitproSet->GetTileToSlotMap().empty())
		{
			return;
		}


		vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_COMPUTE,
			s_bindlessDescitproSet->GetEffectsPipeline());

		VkDescriptorSet set0 = s_bindlessDescitproSet->GetComputeDescriptorSetFrame(frameIndex);
		vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_COMPUTE,
			s_bindlessDescitproSet->GetEffectsPipelineLayout(), 0, 1, &set0, 0, nullptr);

		//Gather active slots
		std::unordered_set<uint32_t> uniqueSlots;
		uniqueSlots.reserve(s_bindlessDescitproSet->GetTileToSlotMap().size());

		for (const auto& kv : s_bindlessDescitproSet->GetTileToSlotMap())
		{
			uniqueSlots.insert(kv.second);
		}

		CollisionResultBuffer           collisionResult = {};
		void* data = nullptr;
		vkMapMemory(m_device, m_vulkanGraphicsPipelines->GetGPUCollisionMemory(), 0, sizeof(collisionResult), 0, &data);
		memcpy(&collisionResult, data, sizeof(collisionResult));
		vkUnmapMemory(m_device, m_vulkanGraphicsPipelines->GetGPUCollisionMemory());

		const uint32_t numberOfCollisions = std::min(collisionResult.collisionCount, (uint32_t)MAX_COLLISION_RESULTS);

		CollisionResultsCPU::LatestProjectiles.clear();
		CollisionResultsCPU::LatestProjectiles.reserve(MAX_COLLISION_RESULTS);
		std::vector<glm::vec2> hitsW;
		std::vector<float>     radiiW;
		std::vector<uint32_t>     damages;
		hitsW.reserve(numberOfCollisions);
		radiiW.reserve(numberOfCollisions);

		// this should be tested with two collisions in one frame
		for (uint32_t i = 0; i < numberOfCollisions; ++i)
		{
			const auto& r = collisionResult.results[i];

			if (r.collisionDetected == 0xFFFFFFFFu) continue;

			if (collisionResult.collisionCount > 1)
			{
				EE_CORE_INFO("test more than one collision: {}", r.collisionDetected);

			}

			EE_CORE_INFO("collided to slot: {}, position {}, {}", r.collisionDetected, r.CollisionPosition.x, r.CollisionPosition.y);
			Collision coll{};
			coll.EntityID = (uint64_t(r.hitProjectileID_High) << 32) | uint64_t(r.hitProjectileID_Low);
			coll.HitPosition = r.CollisionPosition;
			coll.Health = r.Health;
			coll.RadiusWS = r.DestructionRadius;
			CollisionResultsCPU::LatestProjectiles.push_back(coll);

			const glm::vec2 W = r.CollisionPosition;
			const float     R = r.DestructionRadius;
			uint32_t damage = r.Damage;
			hitsW.push_back(W);
			radiiW.push_back(R);
			damages.push_back(damage);
		}

		data = nullptr;
		VkDeviceSize size = sizeof(CollisionResultBuffer);
		vkMapMemory(m_device, m_vulkanGraphicsPipelines->GetGPUCollisionMemory(), 0, size, 0, &data);
		// zero everything
		std::memset(data, 0, size);

		// then set the claim sentinels for active projectiles
		auto* buf = reinterpret_cast<CollisionResultBuffer*>(data);
		for (uint32_t i = 0; i < 32; ++i) {
			buf->results[i].collisionDetected = 0xFFFFFFFFu; // NO_CLAIM
			buf->results[i].DestructionRadius = 0xFFFFFFFFu; // optional, nice to have
			buf->results[i].Damage = 0u;
		}

		vkUnmapMemory(m_device, m_vulkanGraphicsPipelines->GetGPUCollisionMemory());


		const int FX_W = 4096;               // FX texture width in pixels
		const int FX_H = 4096;
		const int   tileW = TILE_PIXEL_WIDTH;
		const int   tileH = TILE_PIXEL_HEIGHT;
		const float pixelSizeWorld = (tileW > 0) ? float(TILE_SIZE) / float(tileW) : 1.0f;
		const float fxCellW_World = FX_W * pixelSizeWorld;
		const float fxCellH_World = FX_H * pixelSizeWorld;

		// local size matches shader (16x16)
		static constexpr uint32_t kLocalX = 16;
		static constexpr uint32_t kLocalY = 16;
		auto CeilDiv = [](uint32_t n, uint32_t d) { return (n + d - 1) / d; };

		// For per-layer barriers
		VkImage colorArray = s_bindlessDescitproSet->GetColorImageArray();
		VkImage propsArray = s_bindlessDescitproSet->GetPropsArrayImage();

		std::vector<AffectedTile> affectedTiles;
		BuildAffectedTilesCPU(hitsW, radiiW, damages, uniqueSlots, 
			pixelSizeWorld, tileW, tileH, affectedTiles);

		const bool yDown = false;
		glm::vec2 fxGridTopLeftW(std::numeric_limits<float>::infinity(),
			yDown ? std::numeric_limits<float>::infinity()
			: -std::numeric_limits<float>::infinity());		// Transition textures to GENERAL layout

		for (size_t i = 0; i < CHUNK_GRID_SIZE; i++)
		{
			VulkanTexture& tex = *s_VulkanData.VisualEffectsTextureSlots[i];
			glm::vec2 texOriginW = tex.GetTextureOrigin(); 
			texOriginW.x = texOriginW.x - 0.5f * CHUNK_SIZE;
			texOriginW.y = texOriginW.y + 0.5f * CHUNK_SIZE;

			fxGridTopLeftW.x = std::min(fxGridTopLeftW.x, texOriginW.x);
			fxGridTopLeftW.y = yDown ? std::min(fxGridTopLeftW.y, texOriginW.y)
				: std::max(fxGridTopLeftW.y, texOriginW.y);
	
		}

		
		//  Dispatch tiles that were affected by collision/destruction
		for (AffectedTile tile : affectedTiles)
		{
			glm::vec2 tileOriginW = s_VulkanBindlessData.m_slotOriginWorld[tile.slot];
			const int    FX_TEXTURE_HEIGHT = s_VulkanData.VisualEffectsTextureSlots[0]->GetHeight(); // they should be same size all
			const int    FX_TEXTURE_WIDTH = s_VulkanData.VisualEffectsTextureSlots[0]->GetWidth();

			uint32_t fxIdx = VulkanUtils::TileToFXIndex(tileOriginW, fxGridTopLeftW, pixelSizeWorld,
				FX_TEXTURE_WIDTH, FX_TEXTURE_HEIGHT, /*worldYDown=*/false);
			if (fxIdx > CHUNK_GRID_SIZE)
			{
				// tile is not inside the grid.
				continue;
			}

			// Build push constants for THIS tile
			EffectPushConstants pc{};
			pc.textureIndex = tile.slot;
			pc.textureOrigin = s_VulkanBindlessData.m_slotOriginWorld[tile.slot]; // top-left in world
			pc.pixelSize = pixelSizeWorld;

			// keep your effect params:
			pc.defaultTimer = s_effectPushConstants.defaultTimer;
			pc.glowStrength = s_effectPushConstants.glowStrength;
			pc.maxTimer = s_effectPushConstants.maxTimer;
			pc.flags = s_effectPushConstants.flags;
			pc.impactTint = s_effectPushConstants.impactTint;
			pc.destroyedTint = s_effectPushConstants.destroyedTint;
			pc.flashTint = s_effectPushConstants.flashTint;
			pc.effectParams0 = s_effectPushConstants.effectParams0;

			glm::vec2 texOriginW = s_VulkanData.VisualEffectsTextureSlots[fxIdx]->GetTextureOrigin();
			pc.fxIdx = fxIdx;

			const int col = fxIdx % 3;
			const int row = fxIdx / 3;
			const float fxPxW = pixelSizeWorld;
			
			glm::vec2 cellSizeW = glm::vec2(FX_TEXTURE_WIDTH, FX_TEXTURE_HEIGHT) * fxPxW;
			glm::vec2 topLeftW = fxGridTopLeftW
				+ glm::vec2(col * cellSizeW.x, -row /* flip y */  * cellSizeW.y);

			pc.fxTextureOrigin = topLeftW;
			pc.hitDamage = tile.totalDamage;
			pc.hitRadiusWS = tile.maxRadius;
			pc.mode = 0; // destruction and init effect

			// Push constants
			vkCmdPushConstants(cmd,
				s_bindlessDescitproSet->GetEffectsPipelineLayout(),
				VK_SHADER_STAGE_COMPUTE_BIT,
				0, sizeof(EffectPushConstants), &pc);

			// Dispatch full tile (or use a content rect if you have one)
			const uint32_t gx = CeilDiv(uint32_t(tileW), kLocalX);
			const uint32_t gy = CeilDiv(uint32_t(tileH), kLocalY);
			vkCmdDispatch(cmd, gx, gy, 1);
		
		}

		for (uint32_t slot : uniqueSlots)
		{
			// transition of all images
			BarrierLayer(cmd, colorArray, slot,
				VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
				VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
				VK_ACCESS_SHADER_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT);

		}


		// Second pass to fade visual effect
		for (size_t fxIdx = 0; fxIdx < CHUNK_GRID_SIZE; fxIdx++)
		{
			const int    FX_TEXTURE_HEIGHT = s_VulkanData.VisualEffectsTextureSlots[0]->GetHeight(); // they should be same size all
			const int    FX_TEXTURE_WIDTH = s_VulkanData.VisualEffectsTextureSlots[0]->GetWidth();

			EffectPushConstants pc{};		
			pc.mode = 1; // effect fade
			pc.fxIdx = fxIdx;
			pc.glowStrength = s_effectPushConstants.glowStrength;
			pc.maxTimer = s_effectPushConstants.maxTimer;

			// Push constants
			vkCmdPushConstants(cmd,
				s_bindlessDescitproSet->GetEffectsPipelineLayout(),
				VK_SHADER_STAGE_COMPUTE_BIT,
				0, sizeof(EffectPushConstants), &pc);

			// Dispatch visual effect texture
			const uint32_t gx = (FX_TEXTURE_HEIGHT + 16 - 1) / 16;
			const uint32_t gy = (FX_TEXTURE_WIDTH + 16 - 1) / 16;
			vkCmdDispatch(cmd, gx, gy, 1);
		}	
	}






	void VulkanRenderer2D::RecordLineCommanedBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex, uint32_t currentFrame)
	{
		EE_PROFILE_FUNCTION();
		if (s_VulkanData.LineVertexCount <= 0)
			return;
		vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_vulkanGraphicsPipelines->GetLinePipeline());

		VkDescriptorSet descriptorSet = m_vulkanGraphicsPipelines->GetLineDescriptorSet(currentFrame);
		vkCmdBindDescriptorSets(
			commandBuffer,
			VK_PIPELINE_BIND_POINT_GRAPHICS,
			m_vulkanGraphicsPipelines->GetLinePipelineLayout(),
			0, 1,
			&descriptorSet,
			0, nullptr
		);

		VkBuffer vertexBuffers[] = { s_VulkanData.LineVertexBuffer->GetBuffer() };

		VkDeviceSize offsets[] = { 0, 0, 0 };
		vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, &offsets[0]);


		vkCmdSetLineWidth(commandBuffer, 3.0f);

		vkCmdDraw(commandBuffer, s_VulkanData.LineVertexCount, 1, 0, 0);

		s_VulkanData.Stats.DrawCalls++;

	}

	void VulkanRenderer2D::ConsumeDestructibleQueue(VkCommandBuffer uploadCB, uint32_t frameIndex)
	{
		std::vector<DestructibleSubmit>& q = s_VulkanBindlessData.submitQueues[frameIndex];
		s_bindlessDescitproSet->BeginFrame(frameIndex, uploadCB);

		const float tileWorldW = float(TILE_SIZE);
		const float tileWorldH = float(TILE_SIZE);

		for (size_t i = 0; i < q.size(); ++i)
		{
			const DestructibleSubmit& s = q[i];

			glm::ivec2 qpos = HashUtils::QuantizeToTile(s.localPos, float(TILE_SIZE));
			const uint64_t uid = s.nameHash;

			const uint32_t slot = s_bindlessDescitproSet->EnsureTileResident(uid, s.atlasUV, uploadCB);

			// CENTER is provided by you:
			const glm::vec2 center = s.worldPos + s.localPos;

			// Painter’s order: sort by “ground” (bottom edge) Y
			const float groundY = center.y * tileWorldH;
			const uint32_t h32 = (uint32_t)((uid ^ (uid >> 32)) * 0x9E3779B1u);
			const float tie = float(h32 & 0x3FF) * 1e-4f;
			const float zKey = groundY * 1024.0f + s.zBias + tie;

			// Pass the real world size so the quad matches exactly
			s_bindlessDescitproSet->AddInstance(center, zKey, slot, 0u);

			// Compute wants bottom-left in world units
			const float tileWorldW = float(TILE_SIZE);
		
			glm::vec2 randomOffset = glm::vec2(0.5f, 0.0f);  // to bottom left tile 128 x 256

			s_VulkanBindlessData.m_slotOriginWorld[slot] = center - randomOffset;
		}
		s_bindlessDescitproSet->EndFrameAndUpload(frameIndex);

		q.clear();
	}




	void VulkanRenderer2D::RecordEditorDrawCommands(VkCommandBuffer commandBuffer, uint32_t imageIndex, uint32_t currentFrame)
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
		
		// 1. Draw ImGui
		if (imguiDrawData != nullptr)
		{
			ImGui_ImplVulkan_RenderDrawData(imguiDrawData, commandBuffer);
		}
	
		vkCmdEndRenderPass(commandBuffer);

	}


	void VulkanRenderer2D::TransitionImageLayout(VkCommandBuffer commandBuffer,	VkImage image,
		VkImageLayout oldLayout, VkImageLayout newLayout)
	{
		EE_PROFILE_FUNCTION();

		if (oldLayout == newLayout)
		{
			return;

		}

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
		else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_GENERAL)
		{
			srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
			dstAccessMask = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT;

			sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
			destinationStage = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT | VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
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
		else if (oldLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_GENERAL)
		{
			srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
			dstAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
			sourceStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
			destinationStage = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
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
		else if (oldLayout == VK_IMAGE_LAYOUT_GENERAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
		{
			srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
			dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
			sourceStage = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
			destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
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



		//EE_CORE_INFO("Transitioning layout from {} to {}", VulkanUtils::LayoutToString(oldLayout), VulkanUtils::LayoutToString(newLayout));

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
	void VulkanRenderer2D::CalculateBoxCollision(const glm::vec2& position, const glm::vec2& size, 
		float rotation, uint64_t entityID, eCollisionType collisionType, uint32_t damage)
	{
		EE_PROFILE_FUNCTION();

		uint32_t index = s_CollisionData.EntitySlotIndex;
		if (index >= MAX_COLLISION_ENTITIES)
		{
			EE_CORE_WARN("Max vehicle collision slots reached!");
			return;
		}
		s_CollisionData.CollisionEntities[index].Type = (uint32_t)collisionType;
		s_CollisionData.CollisionEntities[s_CollisionData.EntitySlotIndex].Damage = damage;

		s_CollisionData.CollisionEntities[index].Position = position;
		s_CollisionData.CollisionEntities[index].Size = size;
		s_CollisionData.CollisionEntities[index].Rotation = rotation;
		s_CollisionData.CollisionEntities[index].ID_Low = static_cast<uint32_t>(entityID & 0xFFFFFFFF);
		s_CollisionData.CollisionEntities[index].ID_High = static_cast<uint32_t>(entityID >> 32);
		s_CollisionData.EntitySlotIndex;
	}



	void VulkanRenderer2D::CalculateCircleCollision(const glm::vec2& colliderPos, const float colliderRadius, uint64_t entityID,
		eCollisionType collisionType, uint32_t damage, const float destructionRadius, glm::vec2  projectileDirection,
		glm::vec2  TargetPositionAtFireTime, float  DistanceToTargetatFireTime, float  TargetPositionHeightZ1)
	{
		EE_PROFILE_FUNCTION();

		uint32_t index = s_CollisionData.EntitySlotIndex;
		if (index >= MAX_COLLISION_ENTITIES)
		{
			EE_CORE_WARN("Max collision slots reached!");
			return;
		}

		s_CollisionData.CollisionEntities[index].Type = (uint32_t)collisionType;
		s_CollisionData.CollisionEntities[s_CollisionData.EntitySlotIndex].Position = colliderPos;
		s_CollisionData.CollisionEntities[s_CollisionData.EntitySlotIndex].Damage = damage;
		s_CollisionData.CollisionEntities[s_CollisionData.EntitySlotIndex].DestructionRadius = destructionRadius;
		s_CollisionData.CollisionEntities[s_CollisionData.EntitySlotIndex].ColliderRadius = colliderRadius;
		s_CollisionData.CollisionEntities[s_CollisionData.EntitySlotIndex].Dir = projectileDirection;
		s_CollisionData.CollisionEntities[s_CollisionData.EntitySlotIndex].EndPos = TargetPositionAtFireTime;
		s_CollisionData.CollisionEntities[s_CollisionData.EntitySlotIndex].RayLen = DistanceToTargetatFireTime;
		s_CollisionData.CollisionEntities[s_CollisionData.EntitySlotIndex].Z1 = TargetPositionHeightZ1;

		s_CollisionData.CollisionEntities[s_CollisionData.EntitySlotIndex].ID_Low = static_cast<uint32_t>(entityID & 0xFFFFFFFF);
		s_CollisionData.CollisionEntities[s_CollisionData.EntitySlotIndex].ID_High = static_cast<uint32_t>(entityID >> 32);
		s_CollisionData.EntitySlotIndex++;
	}

	void VulkanRenderer2D::CalculatePlayerCircleCollision(const glm::vec2& colliderPos, const float colliderRadius, uint64_t entityID, eCollisionType collisionType)
	{
		EE_PROFILE_FUNCTION();

		uint32_t playerIndex = 0; // only one player
		
		s_CollisionData.playerEntities[playerIndex].Position = colliderPos;
		s_CollisionData.playerEntities[playerIndex].ColliderRadius = colliderRadius;
		s_CollisionData.playerEntities[playerIndex].ID_Low = static_cast<uint32_t>(entityID & 0xFFFFFFFF);
		s_CollisionData.playerEntities[playerIndex].ID_High = static_cast<uint32_t>(entityID >> 32);
	}

	void VulkanRenderer2D::DrawTextureQuadWithProperties(const glm::mat4& transform, const std::shared_ptr<VulkanTexture>& texture,const std::shared_ptr<VulkanTexture>& propertiesTexture)
	{
		EE_PROFILE_FUNCTION();

		if (s_VulkanData.QuadIndexCount >= VulkanRenderer2DData::MaxIndices)
		{
			EE_CORE_ASSERT(false, "Quad index count exceeded maximum limit!");
			return;
		}

		if (s_VulkanData.GridSlotIndex >= VulkanRenderer2DData::GridSize)
		{
			//EE_CORE_ASSERT(false, "Texture slot index exceeded maximum limit!");
			return;
		}

		// Use the same slot index for both texture arrays
		float textureIndex = static_cast<float>(s_VulkanData.GridSlotIndex);

		s_VulkanData.GridTextureSlots[s_VulkanData.GridSlotIndex] = texture;
		s_VulkanData.TextureSlots[s_VulkanData.GridSlotIndex] = texture;
		s_VulkanData.propertiesTextureSlots[s_VulkanData.GridSlotIndex] = propertiesTexture;

		s_VulkanData.GridSlotIndex++;

		// Quad vertex data
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

		// Write 4 vertices
		for (size_t i = 0; i < 4; i++)
		{
			glm::vec4 transformed = transform * glm::vec4(quadPositions[i], 1.0f);
			s_VulkanData.QuadVertexBufferPtr->Position = glm::vec3(transformed);
			//s_VulkanData.QuadVertexBufferPtr->Color = tintColor;
			s_VulkanData.QuadVertexBufferPtr->TexCoord = texCoords[i];
			s_VulkanData.QuadVertexBufferPtr->TexIndex = textureIndex;
			//s_VulkanData.QuadVertexBufferPtr->TilingFactor = tilingFactor;
			s_VulkanData.QuadVertexBufferPtr++;
		}

		s_VulkanData.QuadIndexCount += 6;
		s_VulkanData.Stats.QuadCount++;
	}


	void VulkanRenderer2D::DrawTextureQuad(const glm::mat4& transform, const std::shared_ptr<VulkanTexture>& texture, float tilingFactor, const glm::vec4& tintColor)
	{
		EE_PROFILE_FUNCTION();

		if (s_VulkanData.QuadIndexCount >= VulkanRenderer2DData::MaxIndices)
		{
			EE_CORE_ASSERT(false, "Quad index count exceeded maximum limit!");
		}

		if (s_VulkanData.TextureSlotIndex >= VulkanRenderer2DData::MaxTextureSlots)
		{
			EE_CORE_WARN("Texture slot index exceeded maximum limit!");
			return;
		}


		if (s_VulkanData.TextureSlotIndex + s_VulkanData.VisualTextureSlotIndex >= VulkanRenderer2DData::MaxTextureSlots)
		{
			// im add visual textures at the back of textureslots.
			EE_CORE_WARN("VisualTextureSlotIndex + TextureSlotIndex slot index exceeded maximum limit!");
			return;
		}
		



		// Try to get texture slot from map
		float textureIndex = 0.0f;
		textureIndex = static_cast<float>(s_VulkanData.TextureSlotIndex);
		s_VulkanData.TextureSlots[s_VulkanData.TextureSlotIndex] = texture;
		s_VulkanData.TextureSlotIndex++;


		// Quad vertex data
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

		// Write 4 vertices
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

		s_VulkanData.Stats.QuadCount++;
	}


	void VulkanRenderer2D::DrawVisualEffectTexture(const glm::mat4& transform, const std::shared_ptr<VulkanTexture>& texture)
	{
		EE_PROFILE_FUNCTION();


		if (s_VulkanData.VisualTextureSlotIndex >= VulkanRenderer2DData::GridSize)
		{
			EE_CORE_ASSERT(false, "visual Texture slot index exceeded maximum limit!");
			return;
		}


		// Try to get texture slot from map
		float textureIndex = 0.0f;
		textureIndex = static_cast<float>(s_VulkanData.VisualTextureSlotIndex);
		s_VulkanData.VisualEffectsTextureSlots[s_VulkanData.VisualTextureSlotIndex] = texture;
		s_VulkanData.VisualTextureSlotIndex++;

		
		// Quad vertex data
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

		// Write 4 vertices
		for (size_t i = 0; i < 4; i++)
		{
			glm::vec4 transformed = transform * glm::vec4(quadPositions[i], 1.0f);
			s_VulkanData.QuadVertexBufferPtr->Position = glm::vec3(transformed);
			s_VulkanData.QuadVertexBufferPtr->Color = glm::vec4(1);
			s_VulkanData.QuadVertexBufferPtr->TexCoord = texCoords[i];


			s_VulkanData.QuadVertexBufferPtr->TexIndex = textureIndex + s_VulkanData.TextureSlotIndex;
			s_VulkanData.QuadVertexBufferPtr->TilingFactor = 1.0f;
			s_VulkanData.QuadVertexBufferPtr++;
		}

		s_VulkanData.QuadIndexCount += 6;

		s_VulkanData.Stats.QuadCount++;
		
	}


	void VulkanRenderer2D::DrawProjectile(const glm::mat4& transform, const std::shared_ptr<VulkanTexture>& texture, const glm::vec4& tintColor)
	{
		EE_PROFILE_FUNCTION();

		if (s_VulkanProjectileData.QuadIndexCount >= VulkanRenderer2DData::MaxIndices)
		{
			EE_CORE_ASSERT(false, "Quad index count exceeded maximum limit!");
		}

		if (s_VulkanProjectileData.TextureSlotIndex >= VulkanRenderer2DData::MaxTextureSlots)
		{
			EE_CORE_ASSERT(false, "Texture slot index exceeded maximum limit!");
		}
		


		// Try to get texture slot from map
		float textureIndex = 0.0f;
		textureIndex = static_cast<float>(s_VulkanProjectileData.TextureSlotIndex);
		s_VulkanProjectileData.TextureSlots[s_VulkanProjectileData.TextureSlotIndex] = texture;

		s_VulkanProjectileData.TextureSlotIndex++;

		// Quad vertex data
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

		// Write 4 vertices
		for (size_t i = 0; i < 4; i++)
		{
			glm::vec4 transformed = transform * glm::vec4(quadPositions[i], 1.0f);
			s_VulkanProjectileData.QuadVertexBufferPtr->Position = glm::vec3(transformed);
			s_VulkanProjectileData.QuadVertexBufferPtr->Color = tintColor;
			s_VulkanProjectileData.QuadVertexBufferPtr->TexCoord = texCoords[i];
			s_VulkanProjectileData.QuadVertexBufferPtr->TexIndex = textureIndex; // CHANGE
			s_VulkanProjectileData.QuadVertexBufferPtr++;
		}

		s_VulkanProjectileData.QuadIndexCount += 6;
	}


	void VulkanRenderer2D::DrawQuad(const glm::mat4& transform, const glm::vec4& color, int entityID)
	{
		EE_PROFILE_FUNCTION();

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

		const float textureIndex = 0.0f; 
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

	void VulkanRenderer2D::DrawLineRect(const glm::mat4& transform, const glm::vec4& color, int entityID)
	{
		for (size_t i = 0; i < 4; i++)
		{
			glm::vec3 p0 = glm::vec3(transform * glm::vec4(s_VulkanData.QuadVertexPositions[i], 1.0f));
			glm::vec3 p1 = glm::vec3(transform * glm::vec4(s_VulkanData.QuadVertexPositions[(i + 1) % 4], 1.0f));
			DrawLine(p0, p1, color, entityID);
		}
	}


	void VulkanRenderer2D::DrawLine(const glm::vec3& p0, const glm::vec3& p1, const glm::vec4& color, int entityID)
	{
		if (s_VulkanData.LineVertexCount >= VulkanRenderer2DData::MaxLineVertices)
		{
			NextBatch(); // flush and start new batch
		}

		s_VulkanData.LineVertexBufferPtr->Position = p0;
		s_VulkanData.LineVertexBufferPtr->Color = color;
		//s_VulkanData.LineVertexBufferPtr->EntityID = entityID;
		s_VulkanData.LineVertexBufferPtr++;

		s_VulkanData.LineVertexBufferPtr->Position = p1;
		s_VulkanData.LineVertexBufferPtr->Color = color;
		//s_VulkanData.LineVertexBufferPtr->EntityID = entityID;
		s_VulkanData.LineVertexBufferPtr++;

		s_VulkanData.LineVertexCount += 2;
		s_VulkanData.Stats.LineCount++;

	}


	void VulkanRenderer2D::DrawTile(const glm::vec2& worldPos, const glm::vec4& uv, const glm::vec4& color)
	{
		const float aspect = 2.0f;                 // 128x256
		const float widthWorld = float(TILE_SIZE);
		const float heightWorld = widthWorld * aspect;

		// bottom-center pivot: translate up by half height
		glm::mat4 transform =
			glm::translate(glm::mat4(1.0f), glm::vec3(worldPos + glm::vec2(0.0f, heightWorld * 0.5f), 0.0f)) *
			glm::scale(glm::mat4(1.0f), glm::vec3(widthWorld, heightWorld, 1.0f));

		// Find or bind the atlas texture slot
		float textureIndex = -1.0f;
		for (uint32_t i = 0; i < s_VulkanData.TextureSlotIndex; i++)
		{
			if (s_VulkanData.TextureSlots[i] == AssetManager::GetTileTextureIconAtlas())
			{
				textureIndex = float(i);
				break;
			}
		}
		if (textureIndex < 0.0f)
		{
			textureIndex = float(s_VulkanData.TextureSlotIndex);
			s_VulkanData.TextureSlots[s_VulkanData.TextureSlotIndex] = AssetManager::GetTileTextureIconAtlas();
			s_VulkanData.TextureSlotIndex++;
		}

		// Unit quad centered at origin
		const glm::vec3 quadPositions[4] = {
			{-0.5f, -0.5f, 0.0f},
			{ 0.5f, -0.5f, 0.0f},
			{ 0.5f,  0.5f, 0.0f},
			{-0.5f,  0.5f, 0.0f}
		};

		const glm::vec2 texCoords[4] = {
			{uv.x, uv.y},   // top-left
			{uv.z, uv.y},   // top-right
			{uv.z, uv.w},   // bottom-right
			{uv.x, uv.w}    // bottom-left
		};

		for (int i = 0; i < 4; ++i)
		{
			glm::vec4 transformed = transform * glm::vec4(quadPositions[i], 1.0f);
			s_VulkanData.QuadVertexBufferPtr->Position = glm::vec3(transformed);
			s_VulkanData.QuadVertexBufferPtr->Color = color;
			s_VulkanData.QuadVertexBufferPtr->TexCoord = texCoords[i];
			s_VulkanData.QuadVertexBufferPtr->TexIndex = textureIndex;
			s_VulkanData.QuadVertexBufferPtr->TilingFactor = 1.0f;
			s_VulkanData.QuadVertexBufferPtr++;
		}

		s_VulkanData.QuadIndexCount += 6;
	}




	void VulkanRenderer2D::DrawTile(const glm::vec3& worldPos, const glm::vec4& uv, const glm::vec4& color)
	{
		
		// Assuming you already have a texture atlas bound (e.g., m_tileTextureAtlas)
		float textureIndex = 0.0f;

		for (uint32_t i = 0; i < s_VulkanData.TextureSlotIndex; i++)
		{
			if (s_VulkanData.TextureSlots[i] == AssetManager::GetTileTextureIconAtlas())
			{
				textureIndex = (float)i;
				break;
			}
		}

		// Not yet bound? Add it
		if (textureIndex == 0.0f)
		{
			textureIndex = (float)s_VulkanData.TextureSlotIndex;
			s_VulkanData.TextureSlots[s_VulkanData.TextureSlotIndex] = AssetManager::GetTileTextureIconAtlas();
			s_VulkanData.TextureSlotIndex++;
		}
		// Vertex data (inside DrawQuad or similar):
		const glm::vec3 quadPositions[4] = {
			{-0.5f, -0.5f, 0.0f},
			{ 0.5f, -0.5f, 0.0f},
			{ 0.5f,  0.5f, 0.0f},
			{-0.5f,  0.5f, 0.0f}
		};

		const glm::vec2 texCoords[4] = {
			{uv.x, uv.y},   // top-left
			{uv.z, uv.y},   // top-right
			{uv.z, uv.w},   // bottom-right
			{uv.x, uv.w}    // bottom-left
		};

		glm::mat4 model =
			glm::translate(glm::mat4(1.0f), worldPos) *
			glm::scale(glm::mat4(1.0f), glm::vec3(TILE_SIZE, TILE_SIZE, 1.0f));

		for (int i = 0; i < 4; ++i)
		{
			glm::vec4 transformed = model * glm::vec4(quadPositions[i], 1.0f);
			s_VulkanData.QuadVertexBufferPtr->Position = glm::vec3(transformed);
			s_VulkanData.QuadVertexBufferPtr->Color = color;
			s_VulkanData.QuadVertexBufferPtr->TexCoord = texCoords[i];
			s_VulkanData.QuadVertexBufferPtr->TexIndex = textureIndex;
			s_VulkanData.QuadVertexBufferPtr->TilingFactor = 1.0f;
			s_VulkanData.QuadVertexBufferPtr++;
		}

		s_VulkanData.QuadIndexCount += 6;
	}


	void VulkanRenderer2D::BeginScene(const Camera& camera, const glm::mat4& transform)
	{
		EE_PROFILE_FUNCTION();

		
		//StartBatch();
		s_VulkanData.CameraBuffer.ViewProjection = camera.GetViewProjection() * glm::inverse(transform);
		//s_VulkanData.CameraBuffer.SetData(&s_Data.CameraBuffer, sizeof(Renderer2DData::CameraData));
		//StartBatch();

	}

	void VulkanRenderer2D::BeginScene(const EditorCamera& camera)
	{
		EE_PROFILE_FUNCTION();

		s_VulkanData.CameraBuffer.ViewProjection = camera.GetViewProjection();
		//s_VulkanData.CameraBuffer.SetData(&s_VulkanData.CameraBuffer, sizeof(Renderer2DData::CameraData));
		//StartBatch();
	}

	void VulkanRenderer2D::BeginScene(glm::mat4 viewProjectionMatrix)
	{
		s_VulkanData.CameraBuffer.ViewProjection = viewProjectionMatrix;
		//StartBatch();
	}

	void VulkanRenderer2D::BeginScene()
	{
		//StartBatch();
	}

	

	void VulkanRenderer2D::EndScene()
	{
		
		EE_PROFILE_FUNCTION();
		// Flush the batch
		
	}

	void VulkanRenderer2D::SubmitDestructibleTile(const glm::vec2& worldPos, const glm::vec2& localPos, const glm::vec4& atlasUV, uint64_t nameHash, float zBias)
	{

		const size_t fi = static_cast<size_t>(s_VulkanData.CurrentFrame) % MAX_FRAMES_IN_FLIGHT;

		// Get the vector for this frame
		std::vector<DestructibleSubmit>& submitQueue = s_VulkanBindlessData.submitQueues[fi];

		// Push one item
		submitQueue.emplace_back(DestructibleSubmit{worldPos, localPos, atlasUV, nameHash, zBias });
	
		
	}

	void VulkanRenderer2D::SetSlotOriginWorld(uint32_t slot, const glm::vec2& origin)
	{
		
		s_VulkanBindlessData.m_slotOriginWorld[slot] = origin;
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
