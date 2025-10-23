#include "pch.h"
#include "TileCollisionMask.h"
#include <Engine/Platform/Vulkan/VulkanGraphicsPipeline.h>

namespace Engine
{
	// Definitions of static members
	uint32_t TileBlockedMaskCPU::ChunkSize = 0;
	std::unordered_map<glm::ivec2, TileBlockedMaskCPU::TileMask, IVec2Hasher> TileBlockedMaskCPU::ChunkMasks;
	std::vector<uint32_t> TileBlockedMaskCPU::CachedGPUMask;
	std::vector<DirtyRect> TileBlockedMaskCPU::DirtRects;
}
