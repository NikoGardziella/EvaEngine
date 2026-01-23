#include "pch.h"
#include "VulkanRenderer2D.h"

namespace Engine {

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

}