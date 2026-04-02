#include "pch.h"
#include "VulkanRenderer2D.h"
#include "Engine/AssetManager/AssetManager.h"
#include <Engine/Map/Grid/TileCollisionMask.h>
#include <Engine/Math/HashUtils.h>

namespace Engine {


	void VulkanRenderer2D::DrawTile(const glm::vec2& worldPos, const glm::vec4& uv, const glm::vec4& color, float zOverride)
	{
		// Make sure there is room in the current batch
		if (s_VulkanData.QuadIndexCount + 6 > VulkanRenderer2DData::MaxIndices)
		{
			NextBatch();
		}

		const float aspect = 2.0f;
		const float widthWorld = float(TILE_SIZE);
		const float heightWorld = widthWorld * aspect;

		glm::mat4 transform =
			glm::translate(glm::mat4(1.0f), glm::vec3(worldPos + glm::vec2(0.0f, heightWorld * 0.5f), zOverride)) *
			glm::scale(glm::mat4(1.0f), glm::vec3(widthWorld, heightWorld, 1.0f));

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
			if (s_VulkanData.TextureSlotIndex >= VulkanRenderer2DData::MaxTextureSlots)
			{
				NextBatch();
			}

			textureIndex = float(s_VulkanData.TextureSlotIndex);
			s_VulkanData.TextureSlots[s_VulkanData.TextureSlotIndex] = AssetManager::GetTileTextureIconAtlas();
			s_VulkanData.TextureSlotIndex++;
		}

		const glm::vec3 quadPositions[4] = {
			{-0.5f, -0.5f, 0.0f},
			{ 0.5f, -0.5f, 0.0f},
			{ 0.5f,  0.5f, 0.0f},
			{-0.5f,  0.5f, 0.0f}
		};

		const glm::vec2 texCoords[4] = {
			{uv.x, uv.w},
			{uv.z, uv.w},
			{uv.z, uv.y},
			{uv.x, uv.y}
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

	void VulkanRenderer2D::RemoveTilePixels(const uint32_t slot, const uint32_t newSlot, const std::vector<uint32_t>& words, const int cutY)
	{
		TileToDestroy tileToDestroy;
		tileToDestroy.slot = slot;
		tileToDestroy.newSlot = newSlot;
		tileToDestroy.words = words;
		tileToDestroy.cutY = cutY;

		s_VulkanTilesToDestroyData.TilesDestroyQueu.emplace_back(std::move(tileToDestroy));
		s_VulkanTilesToDestroyData.TileToDestroyIndex++;
	}


	void VulkanRenderer2D::ReadBlockedTileMask(std::vector<uint32_t>& outDestroyedMask, uint32_t count)
	{
		void* data;
		vkMapMemory(m_device, m_vulkanGraphicsPipelines->GetBlockedTileMaskMemory(), 0, sizeof(uint32_t) * count, 0, &data);

		// Copy data from GPU buffer memory to CPU vector
		memcpy(outDestroyedMask.data(), data, sizeof(uint32_t) * count);

		vkUnmapMemory(m_device, m_vulkanGraphicsPipelines->GetBlockedTileMaskMemory());
	}




	bool VulkanRenderer2D::ReadDirtyOut()
	{
		EE_PROFILE_FUNCTION();

		if (m_activeSlots.empty())
			return true;

		Engine::DeltaBitReader reader;
		const uint32_t numTiles = MAX_RESIDENT_LAYERS;      // must match the SSBO layout
		const uint32_t tileW = TILE_PIXEL_WIDTH;         // must match shader
		const uint32_t tileH = TILE_PIXEL_HEIGHT;        // must match shader

		if (!reader.Map(m_device, m_vulkanGraphicsPipelines->GetBlockedTileMaskMemory(), // HOST_VISIBLE memory
			numTiles, tileW, tileH,
			/*offsetBytes=*/0))
		{
			return false;
		}



		// Keep this as "affected tiles only"
		Engine::TileBlockedMaskCPU::DirtyTileRuntime.reserve(m_activeSlots.size());

		for (uint32_t i = 0; i < (uint32_t)m_activeSlots.size(); ++i)
		{
			const uint32_t slotId = m_activeSlots[i];

			if (!reader.Any(slotId))
			{
				continue;
			}

			const uint32_t* w = reader.GetTileWords(slotId);
			//EE_CORE_INFO("slot {} first words: {} , {}", slotId, w[0], w[1]);

			DirtyTileRuntime rt{}; // see typedef below
			rt.slot = slotId;
			rt.topLeft = s_VulkanBindlessData.m_slotOriginWorld[slotId];

			// copy packed words (1 bit per pixel: 1 == alive)
			reader.CopyTileWords(slotId, rt.aliveWords);

			rt.aliveCount = reader.CountAlive(slotId);

			//EE_CORE_INFO("alive count: {}, alive words count{}, slot {}, top left: {} | {}",
			//	rt.aliveCount, rt.aliveWords.size(), rt.slot, rt.topLeft.x, rt.topLeft.y);

			Engine::TileBlockedMaskCPU::DirtyTileRuntime.push_back(std::move(rt));
		}

		reader.Unmap();
		return true;
	}

	bool VulkanRenderer2D::ClearAliveBitsHost()
	{
		EE_PROFILE_FUNCTION();

		// The SAME buffer/memory/offset you bind at set=0,binding=4
		VkDeviceMemory mem = m_vulkanGraphicsPipelines->GetBlockedTileMaskMemory();
		VkDeviceSize   off = 0;  // usually 0
		VkDeviceSize   bytes = Engine::VulkanGraphicsPipeline::DIRTYOUT_TOTAL;

		EE_CORE_ASSERT(mem != VK_NULL_HANDLE, "Alive/BlockedTileMask memory is null");
		EE_CORE_ASSERT((off % 4) == 0 && (bytes % 4) == 0, "offset/size must be 4-byte aligned");


		void* p = nullptr;
		VkResult r = vkMapMemory(m_device, mem, off, bytes, 0, &p);
		if (r != VK_SUCCESS) return false;

		std::memset(p, 0, static_cast<size_t>(bytes));

		vkUnmapMemory(m_device, mem);
		return true;
	}





	void VulkanRenderer2D::ConsumeDestructibleQueue(VkCommandBuffer uploadCB, uint32_t frameIndex)
	{
		EE_PROFILE_FUNCTION();
		std::vector<DestructibleSubmit>& submitQueu = s_VulkanBindlessData.submitQueues[frameIndex];

		const float tileWorldW = float(TILE_SIZE);
		const float tileWorldH = float(TILE_SIZE);

		for (size_t i = 0; i < submitQueu.size(); ++i)
		{


			const DestructibleSubmit& submitTile = submitQueu[i];

			//glm::ivec2 qpos = HashUtils::QuantizeToTile(submitTile.localPos, float(TILE_SIZE));
			const uint64_t uid = submitTile.nameHash;




			const uint32_t slot = s_bindlessDescitproRenderer->GetTileSlotWithUid(uid);
			if (slot == UINT32_MAX)
			{
				continue;
			}

			// CENTER is provided by you:
			const glm::vec2 center = submitTile.worldPos + submitTile.localPos;

			// Painter’s order: sort by “ground” (bottom edge) Y
			const float groundY = center.y * tileWorldH;
			const uint32_t h32 = (uint32_t)((uid ^ (uid >> 32)) * 0x9E3779B1u);
			const float tie = float(h32 & 0x3FF) * 1e-6f; // tiny final fallback only

			

			float dirTie = 0.0f;
			switch (submitQueu[i].tileDirection)
			{
			case eTileDirection::North: dirTie = 4.0000f; break;
			case eTileDirection::East:  dirTie = 3.0000f; break;
			case eTileDirection::South: dirTie = 2.0000f; break;
			case eTileDirection::West:  dirTie = 1.0000f; break;
			default:                    dirTie = 0.0000f; break;
			}
			

			const float layerBias = (submitTile.zBias >= 1.0f) ? -100000.0f : 0.0f;
			const float zKey = layerBias + groundY * 1024.0f + dirTie + tie;
			// Pass the real world size so the quad matches exactly
			glm::vec2 size = glm::vec2(TILE_SIZE, TILE_SIZE * 2);

			s_bindlessDescitproRenderer->AddInstance(center, zKey, slot, 0.0f,submitQueu[i].tileDirection, submitTile.outOpaqueMin, submitTile.outOpaqueMax, size, submitTile.flags);

			// Compute wants bottom-left in world units
			const float tileWorldW = float(TILE_SIZE);

			glm::vec2 randomOffset = glm::vec2(0.5f, 0.0f);  // to bottom left tile 128 x 256

			s_VulkanBindlessData.m_slotOriginWorld[slot] = center - randomOffset;
		}

		submitQueu.clear();
	}


}