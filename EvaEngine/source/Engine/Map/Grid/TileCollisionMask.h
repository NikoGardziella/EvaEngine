#pragma once

#include <cstdint>
#include <vector>
#include <unordered_map>
#include <cstring> // for std::memcpy

 // Use <glm/glm.hpp> instead of <glm/fwd.hpp>
#include <Engine/Map/Utils/IVec2Hasher.h>
#include <Engine/Platform/Vulkan/VulkanGraphicsPipeline.h>
#include <Engine/Renderer/Utils/DeltaBitReader.h>

namespace Engine
{
	struct TileBlockedMaskCPU
	{

		// Static variables (declared here, defined in .cpp)
		static std::vector<uint32_t> CachedGPUMask;
		static std::vector<DirtyRect> DirtRects;
		static std::vector<DirtyTileRuntime> DirtyTileRuntime;

		static uint32_t ChunkSize;

		struct TileMask
		{
			std::vector<uint32_t> Mask;

			void Resize(uint32_t size)
			{
				ChunkSize = size;
				Mask.resize(size * size, 0);
			}

			void SetTileBlocked(uint32_t x, uint32_t y)
			{
				if (x >= ChunkSize || y >= ChunkSize) return;
				Mask[y * ChunkSize + x] = 1;
			}

			void SetTileUnblocked(uint32_t x, uint32_t y)
			{
				if (x >= ChunkSize || y >= ChunkSize) return;
				Mask[y * ChunkSize + x] = 0;
			}

			uint32_t GetTile(uint32_t x, uint32_t y) const
			{
				if (x >= ChunkSize || y >= ChunkSize) return 0;
				return Mask[y * ChunkSize + x];
			}
		};
		static std::unordered_map<glm::ivec2, TileMask, IVec2Hasher> ChunkMasks;

	

		// Static methods
		static void Resize(uint32_t chunkSize)
		{
			ChunkSize = chunkSize;
		}

		static void LoadFromGPU(const glm::ivec2& chunkCoord, const std::vector<uint32_t>& gpuMask)
		{
			TileMask& mask = ChunkMasks[chunkCoord];
			mask.Resize(ChunkSize);
			std::memcpy(mask.Mask.data(), gpuMask.data(), sizeof(uint32_t) * ChunkSize * ChunkSize);
		}

		static void SetTileBlocked(const glm::ivec2& chunkCoord, uint32_t x, uint32_t y)
		{
			ChunkMasks[chunkCoord].SetTileBlocked(x, y);
		}

		static void SetTileUnblocked(const glm::ivec2& chunkCoord, uint32_t x, uint32_t y)
		{
			ChunkMasks[chunkCoord].SetTileUnblocked(x, y);
		}

		static uint32_t GetTile(const glm::ivec2& chunkCoord, uint32_t x, uint32_t y)
		{
			auto it = ChunkMasks.find(chunkCoord);
			if (it == ChunkMasks.end()) return 0;
			return it->second.GetTile(x, y);
		}
	};
}
