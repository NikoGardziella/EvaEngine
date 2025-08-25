#pragma once
#include <unordered_set>
#include <glm/glm.hpp>
#include <entt.hpp>
#include <Engine/Map/Utils/IVec2Hasher.h>
#include <Engine/Platform/Vulkan/VulkanTexture.h>
#include <unordered_map>


namespace Engine {
	
	// 3 sub-segments along one chosen side of a cell
	static constexpr int SUBDIVS = 2;
	struct SubCellOBB {
		glm::vec2 center;       // world-space center
		glm::vec2 halfExtents;  // {half-length along edge, half-thickness}
		glm::vec2 tangent;      // unit vector along the edge (A->B)
	};

	enum class TileDir : uint8_t { North, East, South, West };

	enum class FootSide : uint8_t { South = 0, North = 1, East = 2, West = 3 };

	struct FootSegKey {
		glm::ivec2 cell;   // iso cell (u,v)
		uint8_t    side;   // FootSide
		uint8_t    seg;    // 0..FOOT_SUBDIVS-1
	};
	struct FootSegHash {
		size_t operator()(const FootSegKey& k) const noexcept {
			size_t h = std::hash<int>()(k.cell.x) ^ (std::hash<int>()(k.cell.y) << 1);
			h ^= (size_t)k.side + 0x9e3779b97f4a7c15ull + (h << 6) + (h >> 2);
			h ^= (size_t)k.seg + 0x9e3779b97f4a7c15ull + (h << 6) + (h >> 2);
			return h;
		}
	};
	struct FootSegEq {
		bool operator()(const FootSegKey& a, const FootSegKey& b) const noexcept {
			return a.seg == b.seg && a.side == b.side && a.cell == b.cell;
		}
	};




	class GridMap
	{
	public:
		void BuildFromRegistry(entt::registry& registry);
		void GridMap::MarkBlockedSubtilesFromTexture(const glm::vec2& worldPosition,
			const std::vector<uint8_t>& textureData, uint32_t textureWidth, uint32_t textureHeight);

		bool IsBlocked(glm::ivec2 worldTileCoords) const;

		void Clear();

		bool HasLineOfSight(glm::vec2 fromWorld, glm::vec2 toWorld, bool debugDraw);

		void UpdateTiles(const glm::ivec2& centerChunkCoord);

		//void UpdateLOSBlockedTilesFromHealthTexture(const VulkanTexture& healthTexture);


		//bool HasLineOfSight(glm::ivec2 from, glm::ivec2 to);

		static int floorDiv(int a, int b);

		void DrawDebugLine(glm::vec2 from, glm::vec2 to, const glm::vec4& color);

		void DrawDebugBlockedTiles() const;
	private:

		inline FootSide ParseFootSideOrDefault(const std::string& name) {
			// expects "..._N", "..._S", "..._E", "..._W" at the end; default South
			auto pos = name.rfind('_');
			if (pos == std::string::npos || pos + 1 >= name.size()) return FootSide::South;
			switch (std::toupper(name[pos + 1])) {
			case 'N': return FootSide::North;
			case 'E': return FootSide::East;
			case 'W': return FootSide::West;
			case 'S': default: return FootSide::South;
			}
		}
		inline TileDir ParseTileDirFromName(const std::string& name) {
			auto lastU = name.find_last_of('_');
			if (lastU != std::string::npos && lastU + 1 < name.size()) {
				char c = (char)std::toupper(name[lastU + 1]);
				if (c == 'N') return TileDir::North;
				if (c == 'E') return TileDir::East;
				if (c == 'S') return TileDir::South;
				if (c == 'W') return TileDir::West;
			}
			// fallback: South as baseline
			return TileDir::South;
		}

		// If your screen space is Y-down (in 2D), a clockwise turn is a negative angle around +Z
		// Mapping below fixes the “N/S and E/W are reversed” complaint in most Y-down setups.
		inline float TileDirToRadians(TileDir d) {
			switch (d) {
			case TileDir::South: return 0.0f;                          // baseline
			case TileDir::East:  return glm::radians(-90.0f);           // clockwise
			case TileDir::North: return glm::radians(180.0f);           // 180°
			case TileDir::West:  return glm::radians(+90.0f);           // counter-clockwise
			}
			return 0.0f;
		}
	private:
		// remove
		std::unordered_set<glm::ivec2, IVec2Hasher, IVec2Equal> m_blockedTiles;
		std::unordered_set<glm::ivec2, IVec2Hasher, IVec2Equal> m_previousBlockedTiles;
		std::unordered_set<FootSegKey, FootSegHash, FootSegEq> m_blockedFootSegs;
		std::vector<SubCellOBB> m_blockedSubCells; 
	};
}


