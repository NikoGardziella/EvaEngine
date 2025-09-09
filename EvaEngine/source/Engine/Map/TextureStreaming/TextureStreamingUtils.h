#pragma once
#include <glm/fwd.hpp>
#include <glm/common.hpp>
#include "TextureStreamingSystem.h"
#include <Engine/Scene/Scene.h>

namespace Engine {

	
	class TextureStreamingUtils
	{
	public:
		static bool BakeDynamicObjectIfNeeded(Scene* scene, Entity entity);
		static bool BakeRoofTextureIfNeeded(Scene* scene, Entity entity);
		static bool BakeVehicleTextureIfNeeded(Scene* scene, Entity entity);
		static int FloorDiv(int a, int b);
		static void UnpackCategoryFlags(uint8_t a, uint8_t& category, uint8_t& flags);
		static uint8_t PackCategoryFlags(uint8_t category, uint8_t flags);
		static bool AlphaOver(uint8_t sR, uint8_t sG, uint8_t sB, uint8_t sA, uint8_t& dR, uint8_t& dG, uint8_t& dB, uint8_t& dA);
		static bool MergePropertiesPixel(uint8_t sPr, uint8_t sPg, uint8_t sPb, uint8_t sPa, uint8_t sCoverageA, uint8_t& dPr, uint8_t& dPg, uint8_t& dPb, uint8_t& dPa);
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