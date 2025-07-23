#pragma once
#include <glm/ext/vector_int2.hpp>
#include <glm/fwd.hpp>
#include <glm/common.hpp>
#include "TextureStreamingSystem.h"
#include <Engine/Scene/Entity.h>

namespace Engine {


	class TextureStreamingUtils
	{
	public:
		static bool BakeRoofTextureIfNeeded(entt::registry& registry, entt::entity entity);
	};


	namespace MapUtils {
	
	
		inline glm::ivec2 WorldToChunk(glm::vec2 worldPos)
		{
			return glm::floor(worldPos / float(CHUNK_SIZE));
		}

		inline glm::vec2 ChunkToWorld(glm::ivec2 chunkCoord)
		{
			return glm::vec2(chunkCoord) * float(CHUNK_SIZE);
		}
		inline glm::vec2 GetWorldPosition(const glm::vec2& localTilePos, const glm::vec3& entityTranslation)
		{
			return glm::vec2(entityTranslation) + localTilePos * float(TILE_SIZE);
		}

		inline glm::ivec2 GetWorldTileCoords(const glm::vec2& localTilePos, const glm::vec3& entityTranslation)
		{
			glm::vec2 worldPos = glm::vec2(entityTranslation) + localTilePos * float(TILE_SIZE);
			return worldPos;
		}
	}
} 