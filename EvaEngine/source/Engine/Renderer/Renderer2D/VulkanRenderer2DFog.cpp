#include "pch.h"
#include "VulkanRenderer2D.h"

namespace Engine {

	void VulkanRenderer2D::RecordFogOfWarComputeCommandBuffer(VkCommandBuffer cmd, uint32_t frameIndex)
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

		Engine::VulkanFogOfWarPipelines::FogPC fogPC{};
		fogPC.uVP = s_VulkanData.CameraBuffer.ViewProjection;
		fogPC.uInvVP = glm::inverse(s_VulkanData.CameraBuffer.ViewProjection);
		fogPC.playerPos = s_PlayerData.CameraPos;
		fogPC.visRadius = s_PlayerData.visionRadiusW;
		fogPC.time = m_timer;
		fogPC.flags = 0u; // world-space fan

		// 1) FAN -> write stencil = 1 (world-space)
		vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS,
			m_vulkanFogOfWarPipelines->GetStencilWritePipeline());

		vkCmdPushConstants(cmd, m_vulkanFogOfWarPipelines->GetFogOverlayPipelineLayout(),
			VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
			0, sizeof(fogPC), &fogPC);

		if (s_VulkanData.Fog.submitResult.fan.vertexCount > 0)
			vkCmdDraw(cmd,
				s_VulkanData.Fog.submitResult.fan.vertexCount, 1,
				s_VulkanData.Fog.submitResult.fan.firstVertex, 0);

		// 2) QUAD -> draw fog where stencil != 1 (screen-space)
		vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS,
			m_vulkanFogOfWarPipelines->GetFogOverlayPipeline());

		fogPC.flags = 1u; // screen-space quad
		vkCmdPushConstants(cmd, m_vulkanFogOfWarPipelines->GetFogOverlayPipelineLayout(),
			VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
			0, sizeof(fogPC), &fogPC);

		if (s_VulkanData.Fog.submitResult.quad.vertexCount > 0)
			vkCmdDraw(cmd,
				s_VulkanData.Fog.submitResult.quad.vertexCount, 1,
				s_VulkanData.Fog.submitResult.quad.firstVertex, 0);

		float fogMovementSpeed = 0.07f;
		m_timer += fogMovementSpeed;
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