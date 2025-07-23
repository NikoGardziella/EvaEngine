#pragma once
#include <unordered_set>
#include <glm/glm.hpp>
#include <entt.hpp>
#include <Engine/Map/Utils/IVec2Hasher.h>


namespace Engine {

	

	class GridMap
	{
	public:
		void BuildFromRegistry(entt::registry& registry);
		void GridMap::MarkBlockedSubtilesFromTexture(const glm::vec2& worldPosition,
			const std::vector<uint8_t>& textureData, uint32_t textureWidth, uint32_t textureHeight);

		bool IsBlocked(glm::ivec2 worldTileCoords) const;

		void Clear();

		bool HasLineOfSight(glm::vec2 fromWorld, glm::vec2 toWorld, bool debugDraw);

		//bool HasLineOfSight(glm::ivec2 from, glm::ivec2 to);

		void DrawDebugLine(glm::vec2 from, glm::vec2 to, const glm::vec4& color);

		void DrawDebugBlockedTiles() const;


	private:
		std::unordered_set<glm::ivec2, IVec2Hasher> m_blockedTiles;
	};
}


