#include "pch.h"
#include "VulkanRenderer2D.h"
#include "Engine/Renderer/Renderer2D/Utils/Renderer2DUtils.h"
#include <Engine/Renderer/Lights/VulkanLighting.h>

namespace Engine {

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

		if (s_bindlessDescitproRenderer->GetTileToSlotMap().empty())
		{
			return;
		}

		vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_COMPUTE,
			s_bindlessDescitproRenderer->GetEffectsPipeline());


		VkDescriptorSet set0 = s_bindlessDescitproRenderer->GetEffectsDescriptorSet(frameIndex);
		vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_COMPUTE,
			s_bindlessDescitproRenderer->GetEffectsPipelineLayout(), 0, 1, &set0, 0, nullptr);

		//Gather active slots
		std::unordered_set<uint32_t> uniqueSlots;
		uniqueSlots.reserve(s_bindlessDescitproRenderer->GetTileToSlotMap().size());

		for (const auto& kv : s_bindlessDescitproRenderer->GetTileToSlotMap())
		{
			uniqueSlots.insert(kv.second);
		}

		const int   tileW = TILE_PIXEL_WIDTH;
		const int   tileH = TILE_PIXEL_HEIGHT;
		const float pixelSizeWorld = (tileW > 0) ? float(TILE_SIZE) / float(tileW) : 1.0f;

		// local size matches shader (16x16)
		static constexpr uint32_t kLocalX = 16;
		static constexpr uint32_t kLocalY = 16;
		auto CeilDiv = [](uint32_t n, uint32_t d) { return (n + d - 1) / d; };

		// For per-layer barriers
		VkImage colorArray = s_bindlessDescitproRenderer->GetColorImageArray();
		VkImage propsArray = s_bindlessDescitproRenderer->GetPropsArrayImage();

		std::vector<AffectedTile> affectedTiles;

		for (size_t i = 0; i < s_CPUExplosionsData.CPUExplosions.size(); i++)
		{
			m_hitsW.push_back(s_CPUExplosionsData.CPUExplosions[i].HitWorldPos);
			m_radiiW.push_back(s_CPUExplosionsData.CPUExplosions[i].radiWorld);
			m_damages.push_back(s_CPUExplosionsData.CPUExplosions[i].damage);
		}


		BuildAffectedTilesCPU(m_hitsW, m_radiiW, m_damages, uniqueSlots,
			pixelSizeWorld, tileW, tileH, affectedTiles);
		m_activeSlots.resize(affectedTiles.size());


		for (size_t i = 0; i < affectedTiles.size(); i++)
		{
			m_activeSlots[i] = affectedTiles[i].slot;
		}



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
		for (const AffectedTile& tile : affectedTiles)
		{
			// Safety check in debug builds
			EE_CORE_ASSERT(tile.hitIndex < m_hitsW.size(), "hitIndex out of range");

			// You *can* use tile.impactCenterWorld / radiusW / damage directly,
			// or re-read from arrays if you prefer:
			const glm::vec2& hitPos = m_hitsW[tile.hitIndex];
			const float      radiusW = m_radiiW[tile.hitIndex];
			const uint32_t   damage = m_damages[tile.hitIndex];

			glm::vec2 tileOriginW = s_VulkanBindlessData.m_slotOriginWorld[tile.slot];

			EffectPushConstants pc{};
			pc.textureIndex = tile.slot;
			pc.textureOrigin = tileOriginW;
			pc.pixelSize = pixelSizeWorld;

			// Per-hit explosion data:
			pc.impactCenterWorld = hitPos;
			pc.hitRadiusWS = radiusW;
			pc.hitDamage = damage;

			// FX / other params:
			pc.defaultTimer = s_effectPushConstants.defaultTimer;
			pc.glowStrength = s_effectPushConstants.glowStrength;
			pc.maxTimer = s_effectPushConstants.maxTimer;
			pc.flags = s_effectPushConstants.flags;
			pc.impactTint = s_effectPushConstants.impactTint;
			pc.destroyedTint = s_effectPushConstants.destroyedTint;
			pc.flashTint = s_effectPushConstants.flashTint;
			pc.effectParams0 = s_effectPushConstants.effectParams0;

			pc.mode = 0;

			vkCmdPushConstants(cmd,
				s_bindlessDescitproRenderer->GetEffectsPipelineLayout(),
				VK_SHADER_STAGE_COMPUTE_BIT,
				0, sizeof(EffectPushConstants), &pc);

			const uint32_t gx = CeilDiv(uint32_t(tileW), kLocalX);
			const uint32_t gy = CeilDiv(uint32_t(tileH), kLocalY);
			vkCmdDispatch(cmd, gx, gy, 1);
		}


		const int FX_TEXTURE_HEIGHT = s_VulkanData.VisualEffectsTextureSlots[0]->GetHeight();
		const int FX_TEXTURE_WIDTH = s_VulkanData.VisualEffectsTextureSlots[0]->GetWidth();
		const float fxPxW = pixelSizeWorld;
		glm::vec2 cellW = glm::vec2((float)CHUNK_SIZE, (float)CHUNK_SIZE);
		for (size_t i = 0; i < m_hitsW.size(); ++i)
		{
			const glm::vec2& posW = m_hitsW[i];
			const float      radius = m_radiiW[i];
			const uint32_t   damage = m_damages[i];


			int col = (int)glm::floor((posW.x - fxGridTopLeftW.x) / cellW.x);
			int row = (int)glm::floor((fxGridTopLeftW.y - posW.y) / cellW.y);

			uint32_t fxIdx = 0xFFFFFFFFu;

			fxIdx = static_cast<uint32_t>((CHUNK_GRID_WIDTH - 1 - row) * CHUNK_GRID_WIDTH + col);
			if (fxIdx >= CHUNK_GRID_SIZE)
			{
				continue;
			}

			EffectPushConstants pc{};
			pc.textureIndex = 0xFFFFFFFFu;
			pc.textureOrigin = glm::vec2(0.0f); // unused in this mode
			pc.pixelSize = pixelSizeWorld;

			pc.defaultTimer = s_effectPushConstants.defaultTimer;
			pc.glowStrength = s_effectPushConstants.glowStrength;
			pc.maxTimer = s_effectPushConstants.maxTimer;
			pc.flags = s_effectPushConstants.flags;
			pc.impactTint = s_effectPushConstants.impactTint;
			pc.destroyedTint = s_effectPushConstants.destroyedTint;
			pc.flashTint = s_effectPushConstants.flashTint;
			pc.effectParams0 = s_effectPushConstants.effectParams0;

			pc.impactCenterWorld = posW;
			pc.hitDamage = damage;
			pc.hitRadiusWS = radius;

			pc.fxIdx = fxIdx;


			glm::vec2 texOriginW = s_VulkanData.VisualEffectsTextureSlots[fxIdx]->GetTextureOrigin();
			texOriginW.x = texOriginW.x - 0.5f * CHUNK_SIZE;
			texOriginW.y = texOriginW.y + 0.5f * CHUNK_SIZE;
			pc.fxTextureOrigin = texOriginW;
			pc.mode = 3;               // FX-only


			vkCmdPushConstants(cmd,
				s_bindlessDescitproRenderer->GetEffectsPipelineLayout(),
				VK_SHADER_STAGE_COMPUTE_BIT,
				0, sizeof(EffectPushConstants), &pc);

			// FX shader only uses invocation (0,0), so 1x1 is enough
			vkCmdDispatch(cmd, 1, 1, 1);
		}



		auto& queue = s_VulkanTilesToDestroyData.TilesDestroyQueu;
		for (const auto& job : queue)
		{
			const uint32_t slot = job.slot;
			const uint32_t gx = CeilDiv(uint32_t(tileW), kLocalX);
			const uint32_t gy = CeilDiv(uint32_t(tileH), kLocalY);
			EffectPushConstants pc{};
			pc.textureIndex = slot;
			pc.newSlot = job.newSlot;
			pc.mode = 2;
			pc.cutY = job.cutY;
			vkCmdPushConstants(cmd,
				s_bindlessDescitproRenderer->GetEffectsPipelineLayout(),
				VK_SHADER_STAGE_COMPUTE_BIT,
				0, sizeof(EffectPushConstants), &pc);
			vkCmdDispatch(cmd, gx, gy, 1);
		}

		queue.clear();


		for (uint32_t slot : uniqueSlots)
		{
			Render2DUtils::BarrierLayer(cmd, colorArray, slot,
				VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
				VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
				VK_ACCESS_SHADER_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT);
		}


		// Second pass to fade visual effect
		for (size_t fxIdx = 0; fxIdx < CHUNK_GRID_SIZE; fxIdx++)
		{

			EffectPushConstants pc{};
			pc.mode = 1; // effect fade
			pc.fxIdx = fxIdx;
			pc.glowStrength = s_effectPushConstants.glowStrength;
			pc.maxTimer = s_effectPushConstants.maxTimer;

			// Push constants
			vkCmdPushConstants(cmd,
				s_bindlessDescitproRenderer->GetEffectsPipelineLayout(),
				VK_SHADER_STAGE_COMPUTE_BIT,
				0, sizeof(EffectPushConstants), &pc);

			// Dispatch visual effect texture
			const uint32_t gx = (FX_TEXTURE_HEIGHT + 16 - 1) / 16;
			const uint32_t gy = (FX_TEXTURE_WIDTH + 16 - 1) / 16;
			vkCmdDispatch(cmd, gx, gy, 1);
		}


	}

	void VulkanRenderer2D::DrawVisualEffectTexture(const glm::mat4& transform, const std::shared_ptr<VulkanTexture>& texture)
	{
		EE_PROFILE_FUNCTION();

		if (s_VulkanData.VisualTextureSlotIndex >= VulkanRenderer2DData::GridSize)
		{
			//EE_CORE_ASSERT(false, "visual Texture slot index exceeded maximum limit!");
			EE_CORE_WARN("visual Texture slot index exceeded maximum limit!");
			return;
		}

		const uint32_t idx = s_VulkanData.VisualTextureSlotIndex;
		s_VulkanData.VisualEffectsTextureSlots[idx] = texture;
		s_VulkanData.VisualTextureSlotIndex++;

		const float textureIndex = float(idx);
		constexpr float VISUAL_BASE = float(MAX_TEXTURES - CHUNK_GRID_SIZE); // 23 when 32/9

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

		

		for (size_t i = 0; i < 4; i++)
		{
			glm::vec4 transformed = transform * glm::vec4(quadPositions[i], 1.0f);
			s_VulkanData.QuadVertexBufferPtr->Position = glm::vec3(transformed);
			s_VulkanData.QuadVertexBufferPtr->Color = glm::vec4(1);
			s_VulkanData.QuadVertexBufferPtr->TexCoord = texCoords[i];

			// FIX: visual textures live in the BACK of u_Textures[]
			s_VulkanData.QuadVertexBufferPtr->TexIndex = VISUAL_BASE + textureIndex;
			s_VulkanData.QuadVertexBufferPtr->TilingFactor = 1.0f;

			s_VulkanData.QuadVertexBufferPtr++;
		}

		s_VulkanData.QuadIndexCount += 6;
		s_VulkanData.Stats.QuadCount++;
	}



}