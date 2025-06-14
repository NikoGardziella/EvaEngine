#pragma once
#include <glm/ext/vector_int2.hpp>
#include <glm/fwd.hpp>
#include <glm/common.hpp>
#include "TextureStreamingSystem.h"

namespace Engine {


	namespace MapUtils {
	
	
		glm::ivec2 WorldToChunk(glm::vec2 worldPos)
		{
			return glm::floor(worldPos / float(CHUNK_SIZE));
		}

		glm::vec2 ChunkToWorld(glm::ivec2 chunkCoord)
		{
			return glm::vec2(chunkCoord) * float(CHUNK_SIZE);
		}

	}
} 