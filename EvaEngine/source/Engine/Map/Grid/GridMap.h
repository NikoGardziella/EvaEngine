#pragma once
#include <unordered_set>
#include <glm/glm.hpp>
#include <Engine/Map/Utils/IVec2Hasher.h>
#include <vector>
#include "Engine/Map/Grid/GridUtils/GridUtils.h"
#include <Engine/Platform/Vulkan/VulkanGraphicsPipeline.h>
#include <Engine/Core/Core.h>
#include <Engine/Platform/Vulkan/VulkanFogOfWarPipelines.h>
#include <Engine/Scene/SceneCamera.h>

namespace Engine {
	
	// 3 sub-segments along one chosen side of a cell
	static constexpr int SUBDIVS = GRID_SUBDIVISIONS;

	

	class Scene;
	class GridMap
	{
	public:
		void BuildFromRegistry(Scene* scene);
		void GridMap::MarkBlockedSubtilesFromTexture(const glm::vec2& worldPosition,
			const std::vector<uint8_t>& textureData, uint32_t textureWidth, uint32_t textureHeight);


		bool HasLineOfSight(glm::vec2 fromWorld, glm::vec2 toWorld, bool debugDraw);


		void UpdateTiles();

		bool IsCellBlocked(const glm::ivec2& cell) const;

		bool IsPointBlockedWithNormal(const glm::vec2& P, glm::vec2& outNormal) const;


		std::vector<glm::vec2> FindPathWorld(const glm::vec2& startWorld, const glm::vec2& goalWorld) const;


		std::vector<SubCellOBB>& GetGridSubcells() { return  m_blockedSubCells; }
		void DrawDebugBlockedTiles() const;
		void DebugDrawPath(const std::vector<glm::vec3>& path) const;
		
		bool IsPointBlocked_Bucketed(const glm::vec2& P, float padding) const;

		
		
		void SmoothVisibilityPoly(const std::vector<glm::vec2>& cur, std::vector<glm::vec2>& inout, float alpha);
	private:

		void DrawDebugLine(glm::vec2 from, glm::vec2 to, const glm::vec4& color) const;

		static void DrawAABB_LineRect(const glm::vec2& wmin, const glm::vec2& wmax, const glm::vec4& color);
		void PushDirtyDebugRectWorld(const glm::vec2& wmin, const glm::vec2& wmax, const glm::vec4& color);

		void RebuildSubcellBuckets();

	






	
	private:
		static float PxToWorld(float px) { return px / float(TILE_PIXEL_WIDTH); }

	public:
		enum class TileDir : uint8_t { North, East, South, West };


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

		enum : uint32_t { MASK_ALIVE = 1u, MASK_DESTROYED = 2u, MASK_DYNFOOT_HIT = 4u };


	private:

	

		// remove // update LOS before
		std::unordered_set<glm::ivec2, IVec2Hasher, IVec2Equal> m_blockedTiles;
		std::vector<Engine::SubCellOBB> m_blockedSubCells; 
		std::unordered_map<uint64_t, std::vector<int>> m_cellToSubcells;

		std::vector<glm::vec2> m_subMin, m_subMax;

		std::vector<uint32_t> m_subcellHitCount;
		struct DebugAABB {
			glm::vec2 minW;
			glm::vec2 maxW;
			glm::vec4 color; // rgba
		};
		std::vector<DebugAABB> m_debugRects;
	};
}


