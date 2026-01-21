#pragma once
#include <glm/ext/vector_float2.hpp>
#include <Engine/Scene/SceneCamera.h>
#include <Engine/Platform/Vulkan/VulkanFogOfWarPipelines.h>
#include <Engine/Core/Core.h>
#include "Engine/Map/Grid/GridMap.h"

namespace Engine {

	class FogOfWar
	{
	public:

		FogOfWar(Ref<GridMap> gridRef);

		void DrawFogOfWar(glm::vec2 playerPos, const SceneCamera& cam);

	private:
		void SmoothVisibilityPoly(const std::vector<glm::vec2>& cur, std::vector<glm::vec2>& inout, float alpha /*0..1*/);

		void BuildVisibilityPolygon(const glm::vec2& originW, float radiusW, int numRays, std::vector<glm::vec2>& outPtsW) const;

		glm::vec2 RaycastFirstBlock(const glm::vec2& fromWorld, const glm::vec2& dirNorm, float maxDistW, bool* outHit) const;

		void DebugDrawVisibility(glm::vec2 playerPos);

		void DebugDrawVisibilityPoly(const glm::vec2& originW, const std::vector<glm::vec2>& ptsW) const;


		bool IsPointBlocked_Bucketed(const glm::vec2& P, float padding) const;

		void BuildFogFan(const glm::vec2& originW, const std::vector<glm::vec2>& polyPtsW, std::vector<Engine::VulkanFogOfWarPipelines::FogVertex>& outFanTris);

		void BuildFogQuad(const glm::vec2& minW, const glm::vec2& maxW, std::vector<Engine::VulkanFogOfWarPipelines::FogVertex>& outQuadTris);

		void BuildFogQuadNDC(std::vector<Engine::VulkanFogOfWarPipelines::FogVertex>& outQuadTris);

		void DrawDebugLine(glm::vec2 from, glm::vec2 to, const glm::vec4& color) const;


	

	private:

		std::vector<glm::vec2> m_prevPts;
		bool m_hasPrev = false;

		Ref<GridMap> m_gridRef;
	};


}
