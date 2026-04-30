#include "pch.h"
#include "VulkanRenderer2D.h"

#include "Engine/Platform/Vulkan/VulkanContext.h"
#include <Engine/Platform/Vulkan/VulkanGraphicsPipeline.h>
#include "Engine/AssetManager/AssetManager.h"
#include "Engine/Platform/Vulkan/VulkanUtils.h"
#include <Engine/Events/Public/CollisionEvents.h>

#include <backends/imgui_impl_vulkan.h>
#include <Engine/Map/Grid/TileCollisionMask.h>
#include <algorithm>
#include <utility>
#include <Engine/Map/TextureStreaming/TextureStreamingSystem.h>
#include <Engine/Math/HashUtils.h>
#include <Engine/Renderer/UI/VulkanUIRenderer.h>

#include "Engine/Renderer/Renderer2D/Utils/Renderer2DUtils.h"

#include "Engine/Renderer/Lights/VulkanLighting.h"
#include <Engine/Renderer/Lights/GPULightBuffer.h>
#include <Engine/Renderer/Lights/VulkanLighting.cpp>
#include <Engine/Scene/Components/Render/TileComponent.h>
namespace Engine {

	//VulkanRenderer2D::SceneData* VulkanRenderer2D::m_sceneData = new SceneData();
	Engine::VulkanRenderer2DData Engine::VulkanRenderer2D::s_VulkanData;
	Engine::VulkanBindlessRenderer2DData Engine::VulkanRenderer2D::s_VulkanBindlessData;
	Engine::VulkanRenderer2DTileDestructionData Engine::VulkanRenderer2D::s_VulkanTilesToDestroyData;
	Ref<VulkanBindlessDescriptorSetRenderer> VulkanRenderer2D::s_bindlessDescitproRenderer;
	CollisionData Engine::VulkanRenderer2D::s_CollisionData;
	CPUExplosionData Engine::VulkanRenderer2D::s_CPUExplosionsData;
	PlayerData Engine::VulkanRenderer2D::s_PlayerData;

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

		m_vulkanFogOfWarPipelines->Destroy();


		
		auto& fog = s_VulkanData.Fog;
		if (fog.mapped)
		{
			vkUnmapMemory(m_device, fog.memory);
			fog.mapped = nullptr;
		}
		if (fog.buffer) vkDestroyBuffer(m_device, fog.buffer, nullptr);
		if (fog.memory) vkFreeMemory(m_device, fog.memory, nullptr);

		fog.buffer = VK_NULL_HANDLE;
		fog.memory = VK_NULL_HANDLE;
		fog.capacityVertices = 0;
		fog.cursorVertices = 0;
		


	}

	void VulkanRenderer2D::Init(Ref<VulkanShadowMap> shadowMap)
	{
		m_shadowMap = shadowMap;
		m_vulkanContext = VulkanContext::Get();
		m_device = m_vulkanContext->GetDeviceManager().GetDevice();

		m_swapchain = m_vulkanContext->GetVulkanSwapchain().GetSwapchain();
		m_swapchainExtent = m_vulkanContext->GetVulkanSwapchain().GetSwapchainExtent();
		m_vulkanGraphicsPipelines = std::make_shared<VulkanGraphicsPipeline>(*m_vulkanContext);
		
	
		// Allocate command buffers and sync objects
		CreateSyncObjects();


		m_camera = std::make_shared<OrthographicCamera>(-5.0f, 5.0f, -5.0f, 5.0f);
		m_camera->SetPosition({ 0.0f, 0.0f, 1.0f }); // Move the camera back to see the quad

		// Update the uniform buffer with the camera's view-projection matrix
		for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
		{
			m_vulkanGraphicsPipelines->UpdateCameraUniformBuffer(i, m_camera->GetViewProjectionMatrix());
		}

		s_VulkanData.LineVertexBufferBase = new VulkanLineVertex[VulkanRenderer2DData::MaxLineVertices];

		/*
		s_VulkanData.LineStagingBuffer = std::make_shared<VulkanBuffer>(
			m_device,
			m_vulkanContext->GetDeviceManager().GetPhysicalDevice(),
			sizeof(VulkanLineVertex) * VulkanRenderer2DData::MaxLineVertices,
			VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
		);

		*/

		//s_VulkanData.LineStagingBuffer->SetData(s_VulkanData.LineVertexBufferBase, sizeof(VulkanLineVertex) * VulkanRenderer2DData::MaxLineVertices);

		{

			s_VulkanData.LineVertexBuffer = std::make_shared<VulkanBuffer>(
				m_device,
				m_vulkanContext->GetDeviceManager().GetPhysicalDevice(),
				sizeof(VulkanLineVertex) * VulkanRenderer2DData::MaxLineVertices,
				VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
				VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
			);
			s_VulkanData.LineVertexBuffer->SetData(s_VulkanData.LineVertexBufferBase, sizeof(VulkanLineVertex) * VulkanRenderer2DData::MaxLineVertices);

			s_VulkanData.LineVertexBufferPtr = s_VulkanData.LineVertexBufferBase;
		
		}

		{
			s_VulkanData.LineVertexBufferBaseUnderlay = new VulkanLineVertex[VulkanRenderer2DData::MaxLineVertices];
			s_VulkanData.LineVertexBufferPtrUnderlay = s_VulkanData.LineVertexBufferBaseUnderlay;
			s_VulkanData.LineVertexCountUnderlay = 0;

			s_VulkanData.LineVertexBufferUnderlay = std::make_shared<VulkanBuffer>(
				m_device,
				m_vulkanContext->GetDeviceManager().GetPhysicalDevice(),
				sizeof(VulkanLineVertex) * VulkanRenderer2DData::MaxLineVertices,
				VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
				VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
			);
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

		
		
		s_VulkanData.TextureSlots[0] = s_VulkanData.WhiteTexture;
		// Clone "logo" texture to slot 1 after loading it once
		// Load original textures (not inserted into slots)
		//AssetManager::AddTexture("logo", Engine::AssetManager::GetAssetPath("textures/ee_logo.png").string(), false);
		AssetManager::AddTexture("ui_weapon_bazooka", Engine::AssetManager::GetAssetPath("textures/UI/HUD/bazooka.png").string(), false);
		AssetManager::AddTexture("ui_weapon_rifle", Engine::AssetManager::GetAssetPath("textures/UI/HUD/Assault_rifle.png").string(), false);
		AssetManager::AddTexture("ui_weapon_grenade", Engine::AssetManager::GetAssetPath("textures/UI/HUD/grenade.png").string(), false);
		AssetManager::AddTexture("ui_weapon_shotgun", Engine::AssetManager::GetAssetPath("textures/UI/HUD/shotgun.png").string(), false);
		
		AssetManager::AddTexture("Idle_gun_000", Engine::AssetManager::GetAssetPath("textures/Idle_gun_000.png").string(), false);
		AssetManager::AddTexture("ee_logo", Engine::AssetManager::GetAssetPath("textures/ee_logo.png").string(), false);
		AssetManager::AddTexture("bullet", Engine::AssetManager::GetAssetPath("textures/Fire_small_asset.png").string(), false);
		AssetManager::AddTexture("zombie1_walk_000", Engine::AssetManager::GetAssetPath("textures/zombie1_walk_000.png").string(), false);
		AssetManager::AddTexture("wall_0019", Engine::AssetManager::GetAssetPath("textures/wall_0019.png").string(), false);
		AssetManager::AddTexture("zombie_walk_000", Engine::AssetManager::GetAssetPath("textures/zombie_walk_000.png").string(), false);
		AssetManager::AddTexture("objects_plant", Engine::AssetManager::GetAssetPath("textures/objects_plant.png").string(), false);
		AssetManager::AddTexture("car_0001", Engine::AssetManager::GetAssetPath("textures/car_0001.png").string(), false);
		AssetManager::AddTexture("house", Engine::AssetManager::GetAssetPath("textures/house.png").string(), false);
		AssetManager::AddTexture("PlayerRunAnimation", Engine::AssetManager::GetAssetPath("animations/player/spritesheet/Run.png").string(), false);

		
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




	

	

	

		

		for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
		{
			m_vulkanGraphicsPipelines->UpdatePlayerCollisionDescriptorSet(i, s_VulkanData.propertiesTextureSlots);
		}



		uint32_t tilesPerRow = CHUNK_SIZE * CHUNK_GRID_WIDTH * GRID_SUBDIVISIONS;
		uint32_t totalTiles = tilesPerRow * tilesPerRow;
		Engine::TileBlockedMaskCPU::CachedGPUMask.resize(totalTiles);


		s_effectPushConstants = VulkanUtils::MakeDefaultEffectsState();


		s_bindlessDescitproRenderer = std::make_shared<VulkanBindlessDescriptorSetRenderer>(m_device, false);


		s_VulkanBindlessData.m_slotOriginWorld.resize(MAX_RESIDENT_LAYERS);


		
		for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
		{
			void* p = nullptr;
			VkDeviceSize sz = sizeof(CollisionResultBuffer);
			vkMapMemory(m_device, m_vulkanGraphicsPipelines->GetGPUCollisionMemory(i), 0, sz, 0, &p);
			m_collisionMapped[i] = static_cast<CollisionResultBuffer*>(p);
		}


		m_uiRenderer = std::make_unique<VulkanUIRenderer>(m_vulkanContext);
		UIRendererInitConfig uiRendererConfig;
		uint32_t maxQuads = 2000;
		uiRendererConfig.maxQuads = maxQuads;
		uiRendererConfig.maxTextures = MAX_UI_TEXTURES;		
		m_uiRenderer->Init(uiRendererConfig, m_vulkanContext);



		Engine::VulkanFogOfWarPipelines::VulkanFogOfWarPipelinesCreateInfo createinfo;

		createinfo.device = m_device;
		createinfo.renderPass = m_vulkanContext->GetGameRenderPass();
		m_vulkanFogOfWarPipelines = std::make_shared<VulkanFogOfWarPipelines>();

		m_vulkanFogOfWarPipelines->Init(createinfo);

		Engine::VulkanRenderer2DData::FogData& fog = s_VulkanData.Fog;

		const uint32_t kMaxVerts = 65536; // enough for 3*numRays + 6
		fog.capacityVertices = kMaxVerts;

		const VkDeviceSize bytes = VkDeviceSize(kMaxVerts) * sizeof(Engine::VulkanFogOfWarPipelines::FogVertex);

		Engine::BufferUtils::CreateBuffer(
			bytes,
			VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
			fog.buffer,
			fog.memory);

		EE_CORE_ASSERT(fog.buffer != VK_NULL_HANDLE, "Fog VB create failed");

		vkMapMemory(m_device, fog.memory, 0, bytes, 0, &fog.mapped);



	

			VulkanLighting::GetLightBuffer() = std::make_shared<VulkanBuffer>(
				m_device,
				m_vulkanContext->GetDeviceManager().GetPhysicalDevice(),
				sizeof(GPULightBuffer),
				VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
				VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
			);

			VkResult mapResult = VulkanLighting::GetLightBuffer()->Map(sizeof(GPULightBuffer), 0);

			if (mapResult != VK_SUCCESS || !VulkanLighting::GetLightBuffer()->Mapped())
			{
				EE_CORE_ERROR("Failed to map light buffer {}");
			}
			else
			{
				EE_CORE_INFO("Light buffer {} mapped successfully at {}",
					VulkanLighting::GetLightBuffer()->Mapped());
			}
			
			for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
			{
				EE_CORE_INFO("Initializing light buffer descriptor for set {}", i);
				s_bindlessDescitproRenderer->UpdateLightBufferDescriptor(i);
				
			}
			m_vulkanGraphicsPipelines->UpdateLightescriptorSets();
			m_vulkanGraphicsPipelines->UpdateShadowMapDescriptorSets(shadowMap);
			VulkanLighting::GetLightSubmitFrameData() = std::make_shared<LightSubmitFrame>();

			s_bindlessDescitproRenderer->UpdateShadowMapDescriptorSets(shadowMap);

			//s_bindlessDescitproRenderer->UpdateVisibilityDescriptorSet(m_vulkanFogOfWarPipelines->GetFogOfWarTexture());

	}




	
	void VulkanRenderer2D::BeginFrame(uint32_t currentFrame)
	{
		EE_PROFILE_FUNCTION();

		TileBlockedMaskCPU::DirtyTileRuntime.clear();


		ReadAndResetCollisionBuffer(currentFrame);

		ReadDirtyOut();

		ClearAliveBitsHost();

		vkWaitForFences(m_device, 1, &m_inFlightFences[currentFrame], VK_TRUE, UINT64_MAX);
		vkResetFences(m_device, 1, &m_inFlightFences[currentFrame]);

		StartBatch();





		s_bindlessDescitproRenderer->BeginFrame(currentFrame);


		m_uiRenderer->BeginFrame(*m_uiRenderer, currentFrame, glm::vec2(m_swapchainExtent.width, m_swapchainExtent.height));

		VulkanLighting::BeginFrame(currentFrame);
	}



	void VulkanRenderer2D::BeginScene(const SceneCamera& camera, const glm::mat4& transform)
	{
		EE_PROFILE_FUNCTION();


		//StartBatch();
		s_VulkanData.CameraBuffer.ViewProjection = camera.GetProjection() * glm::inverse(transform);
		s_VulkanData.CameraBuffer.viewportPx = camera.GetViewportSize();


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



	// rename this to drwaFrame or something
	void VulkanRenderer2D::EndFrame(uint32_t currentFrame, VkCommandBuffer cmd)
	{
		EE_PROFILE_FUNCTION();
		vkResetCommandBuffer(cmd, 0);

		VkCommandBufferBeginInfo beginInfo{};
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

		vkBeginCommandBuffer(cmd, &beginInfo);



		CalculateCollisionFrame(currentFrame, cmd);
		s_VulkanData.CurrentFrame = currentFrame;


		

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




		s_bindlessDescitproRenderer->SetCurrentFrameIndex(currentFrame);
		ConsumeDestructibleQueue(cmd, currentFrame);
		ConsumeAnimationQueue(currentFrame);

		s_bindlessDescitproRenderer->EndFrameAndUpload(currentFrame);

		VulkanLighting::FlushAndUpload(cmd, currentFrame);
		s_bindlessDescitproRenderer->UpdateLightBufferDescriptor(currentFrame);

		

		s_bindlessDescitproRenderer->UpdateTileParams(currentFrame, Engine::VulkanRenderer2D::s_PlayerData);



		Renderer::DrawShadowFrame();

		RenderVisibilityMap(cmd, currentFrame);


		//move somewhere
		s_CollisionData.AffectedSlots.clear();

		s_CollisionData.EntitySlotIndex = 0;
		s_VulkanData.TextureSlotIndex = 0;
		s_VulkanData.GridSlotIndex = 0;
		s_VulkanData.VisualTextureSlotIndex = 0;
		s_VulkanData.TextureToSlotMap.clear();
		s_VulkanBindlessData.submitQueues[currentFrame].clear();


		// --- Begin render pass ---
		VkRenderPassBeginInfo renderPassInfo{};
		renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		renderPassInfo.renderPass = m_vulkanContext->GetGameRenderPass();
		renderPassInfo.framebuffer = m_vulkanContext->GetVulkanSwapchain().GetGameFramebuffer(m_imageIndex);
		renderPassInfo.renderArea = { {0, 0}, m_vulkanContext->GetVulkanSwapchain().GetSwapchainExtent() };

		// Clear color for the color attachment
		VkClearValue clearValues[2] = {};

		// Attachment 0: color
		clearValues[0].color = { { 0.25f, 0.15f, 0.45f, 1.0f } };

		// Attachment 1: depth
		clearValues[1].depthStencil.depth = 1.0f;
		clearValues[1].depthStencil.stencil = 0;

		renderPassInfo.clearValueCount = 2;
		renderPassInfo.pClearValues = clearValues;
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
		if (s_VulkanData.LineVertexCountUnderlay > 0)
		{
			EE_PROFILE_SCOPE("Flush overlay line");

			s_VulkanData.LineVertexBufferUnderlay->SetData(s_VulkanData.LineVertexBufferBaseUnderlay, s_VulkanData.LineVertexCountUnderlay * sizeof(VulkanLineVertex));
		}

		
		m_vulkanGraphicsPipelines->UpdateGameDrawAndVisualImagesDescriptorSets(currentFrame, s_VulkanData.TextureSlots, s_VulkanData.VisualEffectsTextureSlots);

		Draw();
		//RecordFogOfWarComputeCommandBuffer(cmd, currentFrame);

		m_uiRenderer->EndFrame(cmd);


		// End RecordGameDrawCommands render pass
		vkCmdEndRenderPass(cmd);

		RecordPresentDrawCommands(cmd, m_imageIndex, currentFrame);
		RecordEditorDrawCommands (cmd, m_imageIndex, currentFrame);


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





	void VulkanRenderer2D::DeviceWaitIdle()
	{
		vkDeviceWaitIdle(m_device);
	}

	void VulkanRenderer2D::StartBatch()
	{
		EE_PROFILE_FUNCTION();

		s_VulkanData.QuadVertexBufferPtr = s_VulkanData.QuadVertexBufferBase;
		s_VulkanData.QuadIndexCount = 0;
		

		s_VulkanData.LineVertexBufferPtr = s_VulkanData.LineVertexBufferBase;
		s_VulkanData.LineVertexCount = 0;

		s_VulkanData.LineVertexBufferPtrUnderlay = s_VulkanData.LineVertexBufferBaseUnderlay;
		s_VulkanData.LineVertexCountUnderlay = 0;


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

	// this makes no sense. s_VulkanRenderer3D->Draw is called from here
	void VulkanRenderer2D::Draw()
	{
		Renderer::DrawFrame();
	}

	void VulkanRenderer2D::DrawFrame(uint32_t currentFrame, VkCommandBuffer cmd)
	{
		EE_PROFILE_FUNCTION();


		//this can be called multiple times per frame
		RecordGameDrawCommands(cmd, currentFrame);
		RecordLineCommanedBuffer(cmd, m_imageIndex, currentFrame);

		

	}

	void VulkanRenderer2D::DrawTiles(uint32_t currentFrame, VkCommandBuffer cmd, Ref<VulkanShadowMap> shadowMap)
	{

		s_bindlessDescitproRenderer->RecordTiles(cmd, currentFrame,
			s_VulkanData.CameraBuffer.ViewProjection, m_vulkanContext->GetVulkanSwapchain().GetSwapchainExtent(), shadowMap->GetLightSpaceMatrix());
	}

	void VulkanRenderer2D::DrawTilesShadowPass(VkCommandBuffer cmd, uint32_t frameIndex, Ref<VulkanShadowMap> shadowMap)
	{
		// Begin the TILE shadow render pass
		VkRenderPassBeginInfo renderPassInfo{};
		renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		renderPassInfo.renderPass = shadowMap->GetTileShadowmap().renderPass;
		renderPassInfo.framebuffer = shadowMap->GetTileShadowmap().framebuffer;
		renderPassInfo.renderArea.offset = { 0, 0 };
		renderPassInfo.renderArea.extent = {
			shadowMap->GetShadowMapSize(),
			shadowMap->GetShadowMapSize()
		};

		VkClearValue clearValues[2];
		clearValues[0].color = { { 1.0f, 0.0f, 0.0f, 0.0f } }; // color: far depth
		clearValues[1].depthStencil = { 1.0f, 0 };            // depth: far

		renderPassInfo.clearValueCount = 2;
		renderPassInfo.pClearValues = clearValues;

		vkCmdBeginRenderPass(cmd, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

		// Set viewport/scissor
		VkViewport viewport{ 0, 0, (float)shadowMap->GetShadowMapSize(), (float)shadowMap->GetShadowMapSize(), 0.0f, 1.0f };
		vkCmdSetViewport(cmd, 0, 1, &viewport);
		VkRect2D scissor{ {0, 0}, {shadowMap->GetShadowMapSize(), shadowMap->GetShadowMapSize()} };
		vkCmdSetScissor(cmd, 0, 1, &scissor);

		// Draw tiles into the tile shadow map
		s_bindlessDescitproRenderer->DrawTilesShadowPass(cmd, frameIndex,
			shadowMap->GetShadowPipeline()->GetTilesShadowPipeline(),
			shadowMap->GetShadowPipeline()->GetTilesShadowPipelineLayout(),
			shadowMap->GetLightSpaceMatrix(), shadowMap->GetLightDirection());

		vkCmdEndRenderPass(cmd);
	}


	void VulkanRenderer2D::RecordGameDrawCommands(VkCommandBuffer cmd, uint32_t currentFrame)
	{
		EE_PROFILE_FUNCTION();
		

		// 0) Bind the 2D pipeline FIRST
		VkPipeline pipe2D = m_vulkanGraphicsPipelines->GetGamePipeline();
		VkPipelineLayout layout2D = m_vulkanGraphicsPipelines->GetGamePipelineLayout();
		vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipe2D);

		glm::mat4 L = m_shadowMap->GetLightSpaceMatrix();
		



		vkCmdPushConstants(cmd, layout2D, VK_SHADER_STAGE_VERTEX_BIT,
			0, sizeof(glm::mat4), &L);

		// 1) Bind both descriptor sets for the 2D pipeline in one go
		VkDescriptorSet setCamera = m_vulkanGraphicsPipelines->GetCameraDescriptorSet(currentFrame); // set = 0
		VkDescriptorSet setGame = m_vulkanGraphicsPipelines->GetGameDescriptorSet(currentFrame);   // set = 1
		VkDescriptorSet sets[] = { setCamera, setGame };
		vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS,
			layout2D,
			/*firstSet*/ 0,
			/*descriptorSetCount*/ 2,
			sets,
			/*dynamicOffsetCount*/ 0, /*pDynamicOffsets*/ nullptr);

		// 2) VB/IB
		VkBuffer vb = s_VulkanData.QuadVertexBuffer->GetBuffer();
		VkDeviceSize offs = 0;
		vkCmdBindVertexBuffers(cmd, 0, 1, &vb, &offs);
		vkCmdBindIndexBuffer(cmd, s_VulkanData.QuadIndexBuffer->GetBuffer(), 0, VK_INDEX_TYPE_UINT32);

		// 3) Draw
		vkCmdDrawIndexed(cmd, s_VulkanData.QuadIndexCount, 1, 0, 0, 0);

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


		VkDescriptorSet descriptorSet = m_vulkanGraphicsPipelines->GetPresentDescriptorSet(imageIndex); // use imageIndex not currentFrame

		vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_vulkanGraphicsPipelines->GetPresentPipelineLayout(), 0, 1, &descriptorSet, 0, nullptr);

		// hardcoded vertices in fullscreen_shader:
		vkCmdDraw(commandBuffer, 3, 1, 0, 0);

		vkCmdEndRenderPass(commandBuffer);

	}


	

	void VulkanRenderer2D::RecordComputeCommandBuffer(VkCommandBuffer cmd, uint32_t frameIndex)
	{
		// 0) Bind compute pipeline + its bindless compute set
		vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_COMPUTE,
			s_bindlessDescitproRenderer->GetComputePipeline());

		VkDescriptorSet effectsDescriptorSet = s_bindlessDescitproRenderer->GetComputeDescriptorSetFrame(frameIndex);
		vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_COMPUTE,
			s_bindlessDescitproRenderer->GetComputePipelineLayout(),
			0, 1, &effectsDescriptorSet, 0, nullptr);

		
		
		// Dispatch once per slot we rendered this frame
		VkImage colorArray = s_bindlessDescitproRenderer->GetColorImageArray();
		VkImage propsArray = s_bindlessDescitproRenderer->GetPropsArrayImage(); // stays GENERAL; no barrier needed here

		// Dedup slots (m_tileToSlot is uid -> slot)
		if (s_CollisionData.AffectedSlots.empty())
			return;

		std::vector<uint64_t>& UIDs = s_CollisionData.AffectedSlots;

		std::vector<uint32_t> uniqueSlots;
		uniqueSlots.reserve(UIDs.size());

		for (size_t i = 0; i < UIDs.size(); i++)
		{
			uint32_t slot = s_bindlessDescitproRenderer->GetTileSlotWithUid(UIDs[i]);

			if (slot == UINT32_MAX)
				continue;

			uniqueSlots.push_back(slot);
		}

		std::sort(uniqueSlots.begin(), uniqueSlots.end());



		uniqueSlots.erase(
			std::unique(uniqueSlots.begin(), uniqueSlots.end()),
			uniqueSlots.end()
		);
		m_collisionSlotsLastFrame = uniqueSlots;

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
			/*
			Render2DUtils::BarrierLayer(cmd, colorArray, slot,
				VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_LAYOUT_GENERAL,
				VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
				VK_ACCESS_SHADER_READ_BIT, VK_ACCESS_SHADER_WRITE_BIT);

			*/
			// ---- Push & dispatch over the CONTENT area (no rectMin; shader uses contentMinPx+lid) ----
			vkCmdPushConstants(cmd,
				s_bindlessDescitproRenderer->GetComputePipelineLayout(),
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






	void VulkanRenderer2D::BuildAffectedTilesCPU(
		const std::vector<glm::vec2>& hitPositionsW,          // world hits this frame
		const std::vector<float>& radiiW,                     // same length as hits
		const std::vector<uint32_t>& damagesW,                // same length as hits
		const std::vector<uint32_t>& candidateSlots,   // visible/active slots
		float pixelSizeWorld, int tileW, int tileH,
		std::vector<AffectedTile>& outTiles)
	{
		outTiles.clear();

		if (hitPositionsW.empty() ||
			radiiW.size() != hitPositionsW.size() ||
			damagesW.size() != hitPositionsW.size())
			return;

		for (uint32_t slot : candidateSlots)
		{
			const glm::vec2 minW = s_VulkanBindlessData.m_slotOriginWorld[slot]; // top-left in world
			const glm::vec2 maxW = minW + glm::vec2(tileW, tileH) * pixelSizeWorld;

			for (size_t i = 0; i < hitPositionsW.size(); ++i)
			{
				const glm::vec2& hitPos = hitPositionsW[i];
				const float      radiusW = radiiW[i];

				if (Render2DUtils::CircleIntersectsRect_HalfOpen(hitPos, radiusW, minW, maxW))
				{
					AffectedTile t;
					t.slot = slot;
					t.impactCenterWorld = hitPos;          // per-hit center
					t.maxRadius = radiusW;         // per-hit radius
					t.totalDamage = damagesW[i];     // per-hit damage
					t.hitIndex = static_cast<uint32_t>(i);

					outTiles.push_back(t);
				}
			}
		}
	}

	


	void VulkanRenderer2D::ConsumeAnimationQueue(uint32_t frameIndex)
	{
		std::vector<SpriteSubmit>& animationQueu = s_VulkanBindlessData.spriteSubmitQueues[frameIndex];
	
		
		for (const SpriteSubmit& s : animationQueu)
		{
			if (s.slot == UINT32_MAX)
			{
				continue;
			}

			s_bindlessDescitproRenderer->AddSpriteInstance(s.center, s.zKey, s.slot, s.uvMin16, s.uvMax16, s.sizeWorld, s.rotation, eTileDirection::Unknown);
		}

		animationQueu.clear();
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




	uint32_t VulkanRenderer2D::AcquireTextureSlot(const std::shared_ptr<VulkanTexture>& texture)
	{
		EE_CORE_ASSERT(texture, "AcquireTextureSlot: texture is null");

		// Key by pointer (fine as long as texture objects are stable)
		const uint64_t key = (uint64_t)(uintptr_t)texture.get();

		auto it = s_VulkanData.TextureSlotLUT.find(key);
		if (it != s_VulkanData.TextureSlotLUT.end())
			return it->second;

		if (s_VulkanData.TextureSlotIndex >= VulkanRenderer2DData::MaxTextureSlots)
		{
			// In your current architecture you likely "flush" elsewhere.
			// For now, hard-fail or return 0. Better is: FlushAndBeginNewBatch().
			EE_CORE_WARN("MaxTextureSlots exceeded. Need flush/batch break.");
			return 0;
		}

		uint32_t slot = s_VulkanData.TextureSlotIndex++;
		s_VulkanData.TextureSlots[slot] = texture;
		s_VulkanData.TextureSlotLUT[key] = slot;
		return slot;
	}






	void VulkanRenderer2D::EndScene()
	{
		
		EE_PROFILE_FUNCTION();
		// Flush the batch
		
	}

	void VulkanRenderer2D::SubmitDestructibleTile(const glm::vec2& worldPos, const glm::vec2& localPos, const glm::vec4& atlasUV, uint64_t nameHash,
		float zBias, eTileDirection  tileDirection, const glm::ivec2 outOpaqueMin, const glm::ivec2 outOpaqueMax, uint32_t flags, int16_t floor)
	{

		const size_t fi = static_cast<size_t>(s_VulkanData.CurrentFrame) % MAX_FRAMES_IN_FLIGHT;

		// Get the vector for this frame
		std::vector<DestructibleSubmit>& submitQueue = s_VulkanBindlessData.submitQueues[fi];

		// Push one item
		submitQueue.emplace_back(DestructibleSubmit{worldPos, localPos, atlasUV, nameHash, zBias, tileDirection, outOpaqueMin ,outOpaqueMax,flags, floor});
	
		
	}

	void VulkanRenderer2D::SubmitAnimationSpriteInstance(glm::vec2 worldCenter, float zKey, uint32_t spriteSlot,
		glm::uvec2 uvMin16, glm::uvec2 uvMax16, glm::vec2 sizeWorld, float rotation)
	{
		const size_t fi = static_cast<size_t>(s_VulkanData.CurrentFrame) % MAX_FRAMES_IN_FLIGHT;
		auto& q = s_VulkanBindlessData.spriteSubmitQueues[fi];
		q.emplace_back(SpriteSubmit{ worldCenter, zKey,rotation, spriteSlot, uvMin16, uvMax16, sizeWorld });
	}



	void VulkanRenderer2D::SubmitCPUExplosion(glm::vec2 HitWorldPos, float radiWorld, uint32_t damage)
	{
		CPUExplosion cpuexplosion = {};
		cpuexplosion.HitWorldPos = HitWorldPos;
		cpuexplosion.radiWorld = radiWorld;
		cpuexplosion.damage = damage;
		s_CPUExplosionsData.CPUExplosions.push_back(cpuexplosion);
	}

	void VulkanRenderer2D::SubmitPlayerData(PlayerData playerStateData)
	{
		
		s_PlayerData = playerStateData;

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
