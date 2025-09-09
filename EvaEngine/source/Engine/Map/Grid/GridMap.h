#pragma once
#include <unordered_set>
#include <glm/glm.hpp>
#include <Engine/Map/Utils/IVec2Hasher.h>




namespace Engine {
	
	// 3 sub-segments along one chosen side of a cell
	static constexpr int SUBDIVS = GRID_SUBDIVISIONS;
	struct SubCellOBB {
		glm::vec2 center;       // world-space center
		glm::vec2 halfExtents;  // {half-length along edge, half-thickness}
		glm::vec2 tangent;      // unit vector along the edge (A->B)
	};

	

	class Scene;
	class GridMap
	{
	public:
		void BuildFromRegistry(Scene* scene);
		void GridMap::MarkBlockedSubtilesFromTexture(const glm::vec2& worldPosition,
			const std::vector<uint8_t>& textureData, uint32_t textureWidth, uint32_t textureHeight);

		bool IsBlocked(glm::ivec2 worldTileCoords) const;

		void Clear();

		bool HasLineOfSight(glm::vec2 fromWorld, glm::vec2 toWorld, bool debugDraw);


		void UpdateTiles(const glm::ivec2& centerChunkCoord);

		//void UpdateLOSBlockedTilesFromHealthTexture(const VulkanTexture& healthTexture);


		//bool HasLineOfSight(glm::ivec2 from, glm::ivec2 to);

		std::vector<SubCellOBB>& GetGridSubcells() { return  m_blockedSubCells; }


		void DrawDebugLine(glm::vec2 from, glm::vec2 to, const glm::vec4& color);

		void DrawDebugBlockedTiles() const;
	private:
		static glm::vec2 PerpCCW(const glm::vec2& v);
		static bool OBBvsAABB(const SubCellOBB& obb, const glm::vec2& bmin, const glm::vec2& bmax);
		static void SubtileAABB(const glm::ivec2& gs, float subtileSize, glm::vec2& bmin, glm::vec2& bmax);
		static float PxToWorld(float px) { return px / float(TILE_PIXEL_WIDTH); }

	private:
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

		enum : uint32_t { MASK_ALIVE = 1u, MASK_DESTROYED = 2u , MASK_DYNFOOT_HIT = 4u};


	private:
		// remove
		std::unordered_set<glm::ivec2, IVec2Hasher, IVec2Equal> m_blockedTiles;
		std::vector<SubCellOBB> m_blockedSubCells; 
	};
}


