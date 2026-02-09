#include "pch.h"
#include "VulkanRenderer2D.h"

namespace Engine {


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


	void VulkanRenderer2D::DrawTextureQuadWithProperties(const glm::mat4& transform, const std::shared_ptr<VulkanTexture>& texture, const std::shared_ptr<VulkanTexture>& propertiesTexture)
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

	void VulkanRenderer2D::DrawTextureQuad(glm::mat4& transform, const std::shared_ptr<VulkanTexture>& texture, float tilingFactor, const glm::vec4& tintColor)
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


	void VulkanRenderer2D::DrawQuadRaw(const glm::vec3& p0, const glm::vec3& p1, const glm::vec3& p2,
		const glm::vec3& p3, const glm::vec2& uv0, const glm::vec2& uv1, const glm::vec2& uv2, const glm::vec2& uv3,
		const std::shared_ptr<VulkanTexture>& texture, float tilingFactor, const glm::vec4& tintColor)
	{
		EE_PROFILE_FUNCTION();

		if (s_VulkanData.QuadIndexCount >= VulkanRenderer2DData::MaxIndices)
		{
			EE_CORE_ASSERT(false, "Quad index count exceeded maximum limit! Need flush/batch break.");
			return;
		}

		const uint32_t texSlot = AcquireTextureSlot(texture);

		VulkanQuadVertex* v = s_VulkanData.QuadVertexBufferPtr;

		v[0].Position = p0;
		v[0].Color = tintColor;
		v[0].TexCoord = uv0;
		v[0].TexIndex = texSlot;
		v[0].TilingFactor = tilingFactor;

		v[1].Position = p1;
		v[1].Color = tintColor;
		v[1].TexCoord = uv1;
		v[1].TexIndex = texSlot;
		v[1].TilingFactor = tilingFactor;

		v[2].Position = p2;
		v[2].Color = tintColor;
		v[2].TexCoord = uv2;
		v[2].TexIndex = texSlot;
		v[2].TilingFactor = tilingFactor;

		v[3].Position = p3;
		v[3].Color = tintColor;
		v[3].TexCoord = uv3;
		v[3].TexIndex = texSlot;
		v[3].TilingFactor = tilingFactor;

		s_VulkanData.QuadVertexBufferPtr += 4;
		s_VulkanData.QuadIndexCount += 6;
		s_VulkanData.Stats.QuadCount++;
	}

}