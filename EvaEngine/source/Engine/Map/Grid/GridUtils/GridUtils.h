#pragma once
#include <glm/glm.hpp>
#include <Engine/Math/HashUtils.h>

namespace Engine
{
	struct SubCellOBB {
		glm::vec2 center;       // world-space center
		glm::vec2 halfExtents;  // {half-length along edge, half-thickness}
		glm::vec2 tangent;      // unit vector along the edge (A->B)
		uint32_t  TileSlot;

		uint64_t CollisionKey = 0;
		float Health = 1.0f;
	};


	class GridUtils
	{
	public:



		static glm::vec2 PerpCCW(const glm::vec2& v);
		static bool OBBvsAABB(const SubCellOBB& obb, const glm::vec2& bmin, const glm::vec2& bmax);
		static void SubtileAABB(const glm::ivec2& gs, float subtileSize, glm::vec2& bmin, glm::vec2& bmax);

		static bool AABBoverlap(const glm::vec2& amin, const glm::vec2& amax, const glm::vec2& bmin, const glm::vec2& bmax);

		static void OBB_ComputeAABB(const SubCellOBB& obb, glm::vec2& outMin, glm::vec2& outMax);

		static bool PointInSubCellOBB(const glm::vec2& P, const SubCellOBB& obb);

		static bool PointInSubCellOBB_Padded(const glm::vec2& P, const SubCellOBB& obb, float padding);

		static bool OBB_IntersectsCircle(const SubCellOBB& obb, const glm::vec2& C, float R);

		static uint64_t MakeSubCellKey(uint64_t tileUID, uint32_t side, uint32_t subIndex)
		{
			uint64_t h = 1469598103934665603ull;
			HashUtils::HashCombine(h, tileUID);
			HashUtils::HashCombine(h, side);
			HashUtils::HashCombine(h, subIndex);
			return h;
		}

	};

}

