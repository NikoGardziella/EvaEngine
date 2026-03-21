#include "pch.h"
#include "VulkanRenderer2D.h"
#include "Engine/Renderer/Renderer2D/Utils/Renderer2DUtils.h"
#include <Engine/Core/Log.h>

namespace Engine {

	void VulkanRenderer2D::RecordFogOfWarCommandBuffer(VkCommandBuffer cmd, uint32_t frameIndex)
	{
		VkViewport v{};
		v.x = 0.0f; v.y = 0.0f;
		v.width = float(m_swapchainExtent.width);
		v.height = float(m_swapchainExtent.height);
		v.minDepth = 0.0f; v.maxDepth = 1.0f;
		vkCmdSetViewport(cmd, 0, 1, &v);

		VkRect2D s{};
		s.offset = { 0,0 };
		s.extent = m_swapchainExtent;
		vkCmdSetScissor(cmd, 0, 1, &s);

		VkBuffer vb = s_VulkanData.Fog.buffer;
		VkDeviceSize off = 0;
		vkCmdBindVertexBuffers(cmd, 0, 1, &vb, &off);


		if (s_VulkanData.Fog.submitResult.quad.vertexCount > 0)
			vkCmdDraw(cmd,
				s_VulkanData.Fog.submitResult.quad.vertexCount, 1,
				s_VulkanData.Fog.submitResult.quad.firstVertex, 0);

		
	}

	// ********** write pass **********
	void VulkanRenderer2D::RenderVisibilityMap(VkCommandBuffer cmd, uint32_t frameIndex)
	{
		EE_PROFILE_FUNCTION();

		Engine::VulkanRenderer2DData::FogData& fogData = s_VulkanData.Fog;
		VulkanFogOfWarPipelines::FogSubmitResult& r = fogData.submitResult;

		uint32_t width = m_vulkanFogOfWarPipelines->GetFogOfWarTexture()->GetWidth();
		uint32_t height = m_vulkanFogOfWarPipelines->GetFogOfWarTexture()->GetHeight();

		VkClearValue clearValue{};
		clearValue.color = { { 0.0f, 0.0f, 0.0f, 1.0f } };

		VkRenderPassBeginInfo renderPassInfo{ VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO };
		renderPassInfo.renderPass = m_vulkanFogOfWarPipelines->GetVisibilityRenderPass();
		renderPassInfo.framebuffer = m_vulkanFogOfWarPipelines->GetVisibilityFramebuffer();
		renderPassInfo.renderArea.offset = { 0, 0 };
		renderPassInfo.renderArea.extent = { width, height };
		renderPassInfo.clearValueCount = 1;
		renderPassInfo.pClearValues = &clearValue;

		vkCmdBeginRenderPass(cmd, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

		vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_vulkanFogOfWarPipelines->GetVisibilityPipeline());

		VkViewport viewport{ 0.0f, 0.0f, (float)width, (float)height, 0.0f, 1.0f };
		vkCmdSetViewport(cmd, 0, 1, &viewport);
		VkRect2D scissor{ {0, 0}, {width, height} };
		vkCmdSetScissor(cmd, 0, 1, &scissor);

		VulkanFogOfWarPipelines::FogPC fogPC{};
		fogPC.uVP = s_VulkanData.CameraBuffer.ViewProjection;
		fogPC.time = m_timer;
		fogPC.flags = 0u;

		// same as DrawFogOverlay()
		float fogSize = 2.0f;
		float radius = s_PlayerData.visionRadiusW * fogSize;
		fogPC.mapMin = s_PlayerData.PlayerPos - glm::vec2(radius);
		fogPC.mapSize = glm::vec2(radius * fogSize);


		vkCmdPushConstants(cmd,
			m_vulkanFogOfWarPipelines->GetVisibilityPipelineLayout(),
			VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
			0, sizeof(VulkanFogOfWarPipelines::FogPC), &fogPC);


		VkDeviceSize offsets[] = { 0 };
		vkCmdBindVertexBuffers(cmd, 0, 1, &fogData.buffer, offsets);

		if (r.fan.vertexCount > 0)
		{
			vkCmdDraw(cmd,
				fogData.submitResult.fan.vertexCount,
				1,
				fogData.submitResult.fan.firstVertex,
				0);
		}

		vkCmdEndRenderPass(cmd);
	}


	// **********  Reader pass **********
	void VulkanRenderer2D::DrawFogOverlay(VkCommandBuffer cmd)
	{
		vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_vulkanFogOfWarPipelines->GetFogOverlayPipeline());

		VkDescriptorSet set = m_vulkanFogOfWarPipelines->GetVisibilityDescriptorSet();
		vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS,
			m_vulkanFogOfWarPipelines->GetFogOverlayPipelineLayout(),
			0, 1, &set, 0, nullptr);

		uint32_t width = m_swapchainExtent.width;
		uint32_t height = m_swapchainExtent.height;

		VkViewport vp{ 0.0f, 0.0f, (float)width, (float)height, 0.0f, 1.0f };
		vkCmdSetViewport(cmd, 0, 1, &vp);

		VkRect2D sc{ {0,0}, {width, height} };
		vkCmdSetScissor(cmd, 0, 1, &sc);

		VulkanFogOfWarPipelines::FogPC pc{};
		pc.uVP = s_VulkanData.CameraBuffer.ViewProjection;
		pc.time = m_timer;

		// same as RenderVisibilityMap()
		float fogSize = 2.0f;
		float radius = s_PlayerData.visionRadiusW * fogSize;
		pc.mapMin = s_PlayerData.PlayerPos - glm::vec2(radius);
		pc.mapSize = glm::vec2(radius * fogSize);
		

		vkCmdPushConstants(cmd, m_vulkanFogOfWarPipelines->GetFogOverlayPipelineLayout(),
			VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
			0, sizeof(VulkanFogOfWarPipelines::FogPC), &pc);

		// 4. Draw a single Large Triangle or Quad covering the screen
		vkCmdDraw(cmd, 6, 1, 0, 0);
	}



	void VulkanRenderer2D::SubmitFogGeometry(const std::vector<VulkanFogOfWarPipelines::FogVertex>& fanTris, const std::vector<VulkanFogOfWarPipelines::FogVertex>& quadTris)
	{
		Engine::VulkanRenderer2DData::FogData& fog = s_VulkanData.Fog;

		fog.cursorVertices = 0;

		Engine::VulkanFogOfWarPipelines::FogSubmitResult r{};

		auto append = [&](const std::vector<Engine::VulkanFogOfWarPipelines::FogVertex>& src)
			-> Engine::VulkanFogOfWarPipelines::FogDrawRange
			{
				Engine::VulkanFogOfWarPipelines::FogDrawRange out{};
				if (src.empty()) return out;

				const uint32_t need = (uint32_t)src.size();
				if (fog.cursorVertices + need > fog.capacityVertices)
				{
					// In your style: assert or clamp
					EE_CORE_ASSERT(false, "Fog VB overflow");
					return out;
				}

				out.firstVertex = fog.cursorVertices;
				out.vertexCount = need;

				std::memcpy(
					(uint8_t*)fog.mapped + out.firstVertex * sizeof(Engine::VulkanFogOfWarPipelines::FogVertex),
					src.data(),
					need * sizeof(Engine::VulkanFogOfWarPipelines::FogVertex));

				fog.cursorVertices += need;
				return out;
			};

		r.fan = append(fanTris);
		r.quad = append(quadTris);

		s_VulkanData.Fog.submitResult = r;

	}


}