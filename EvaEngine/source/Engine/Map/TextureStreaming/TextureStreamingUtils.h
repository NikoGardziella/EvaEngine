#pragma once
#include <glm/fwd.hpp>
#include <glm/common.hpp>
#include "TextureStreamingSystem.h"
#include <Engine/Scene/Scene.h>

namespace Engine {

	
	class TextureStreamingUtils
	{
	public:
		static bool BakeRoofTextureIfNeeded(Scene* scene, Entity entity);
		static bool BakeVehicleTextureIfNeeded(Scene* scene, Entity entity);
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
			glm::ivec2 tileCoords = glm::ivec2(glm::floor(worldPos / float(TILE_SIZE)));
			return tileCoords;
		}

	}
} 