#include "pch.h"


#include "FogOfWar.h"
#include <Engine/Platform/Vulkan/VulkanFogOfWarPipelines.h>
#include <Engine/Renderer/Renderer2D/VulkanRenderer2D.h>


#include <Engine/Debug/Instrumentor.h>
#include <Engine/Scene/SceneCamera.h>

#include <vector>
#include <Engine/Map/Grid/GridMap.h>
#include <Engine/Renderer/Utils/PlayerData.h>

#include <glm/fwd.hpp>
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>


namespace Engine {



    inline static uint64_t PackXY(int x, int y) {
        return (uint64_t)(uint32_t)x | ((uint64_t)(uint32_t)y << 32);
    }


    FogOfWar::FogOfWar(Ref<GridMap> gridRef)
    {
        m_gridRef = gridRef;
    }

    void FogOfWar::DrawFogOfWar(PlayerData playerStateData, const SceneCamera& cam, const glm::mat4& transform)
    {
        EE_PROFILE_FUNCTION();

        std::vector<glm::vec2> outPtsW;
        float radiusW = playerStateData.visionRadiusW;
        int numRays = 50;

        const glm::mat4 vp = cam.GetProjection() * glm::inverse(transform);

        BuildVisibilityPolygon(playerStateData.PlayerPos, radiusW, numRays, outPtsW, vp);
        //DebugDrawVisibilityPoly(playerPos, outPtsW);

        std::vector<Engine::VulkanFogOfWarPipelines::FogVertex> fanTris, quadTris;


        std::vector<glm::vec2> smoothed = outPtsW;
        SmoothVisibilityPoly(outPtsW, smoothed, 0.50f);// smaller = smoother

        BuildFogFan(playerStateData.PlayerPos, smoothed, fanTris);

        m_currentVisibilityPolygon = smoothed;
       

        //BuildFogQuadNDC(quadTris);
        BuildFogQuad(playerStateData.screenMin, playerStateData.screenMax, quadTris);
        VulkanRenderer2D::SubmitFogGeometry(fanTris, quadTris);
    }

    bool FogOfWar::IsPointVisible(glm::vec2 worldPos) const
    {
        return IsPointInVisibilityPolygon(worldPos, m_currentVisibilityPolygon);
    }

    // Add this to your FogOfWar or GridMap class
    bool FogOfWar::IsPointInVisibilityPolygon(glm::vec2 point, const std::vector<glm::vec2>& visibilityPolygon) const
    {
        if (visibilityPolygon.size() < 3)
            return false;

        // Point-in-polygon test (ray casting algorithm)
        bool inside = false;
        int n = (int)visibilityPolygon.size();

        for (int i = 0, j = n - 1; i < n; j = i++)
        {
            glm::vec2 vi = visibilityPolygon[i];
            glm::vec2 vj = visibilityPolygon[j];

            if ((vi.y > point.y) != (vj.y > point.y) &&
                (point.x < (vj.x - vi.x) * (point.y - vi.y) / (vj.y - vi.y) + vi.x))
            {
                inside = !inside;
            }
        }

        return inside;
    }

    void FogOfWar::SmoothVisibilityPoly(const std::vector<glm::vec2>& cur, std::vector<glm::vec2>& inout, float alpha /*0..1*/)
    {
        if (!m_hasPrev || m_prevPts.size() != cur.size())
        {
            m_prevPts = cur;
            m_hasPrev = true;
            inout = cur;
            return;
        }

        inout.resize(cur.size());
        for (size_t i = 0; i < cur.size(); ++i)
            inout[i] = glm::mix(m_prevPts[i], cur[i], alpha);

        m_prevPts = inout;
    }

    void FogOfWar::BuildVisibilityPolygon(const glm::vec2& originW, float radiusW,
        int numRays, std::vector<glm::vec2>& outPtsW, const glm::mat4& viewProjection) const
    {
        outPtsW.clear();
        outPtsW.reserve((size_t)numRays);

        const float twoPi = 6.28318530718f;

        // Determine the isometric stretch factor
        // move shader 
        glm::vec4 originClip = viewProjection * glm::vec4(originW, 0.0f, 1.0f);
        glm::vec2 originNDC = glm::vec2(originClip.x, originClip.y) / originClip.w;

        // Test how 1 unit in world X maps to screen
        glm::vec4 xClip = viewProjection * glm::vec4(originW.x + 1.0f, originW.y, 0.0f, 1.0f);
        glm::vec2 xNDC = glm::vec2(xClip.x, xClip.y) / xClip.w;
        float xStretch = glm::length(xNDC - originNDC);

        // Test how 1 unit in world Y maps to screen
        glm::vec4 yClip = viewProjection * glm::vec4(originW.x, originW.y + 1.0f, 0.0f, 1.0f);
        glm::vec2 yNDC = glm::vec2(yClip.x, yClip.y) / yClip.w;
        float yStretch = glm::length(yNDC - originNDC);

        // Inverse of the screen stretch
        float radiusX = radiusW * (yStretch / xStretch);
        float radiusY = radiusW;

        for (int i = 0; i < numRays; ++i)
        {
            float a = twoPi * (float(i) / float(numRays));

            // Create ellipse in world space
            float x = std::cos(a) * radiusX;
            float y = std::sin(a) * radiusY;
            glm::vec2 ellipseDir = glm::normalize(glm::vec2(x, y));

            // Raycast with ellipse-adjusted length
            float maxDist = glm::length(glm::vec2(x, y));

            bool hit = false;
            glm::vec2 p = RaycastFirstBlock(originW, ellipseDir, maxDist, &hit);
           

            outPtsW.push_back(p);
        }
    }


    glm::vec2 FogOfWar::RaycastFirstBlock(const glm::vec2& fromWorld,
        const glm::vec2& dirNorm, float maxDistW, bool* outHit) const
    {
        constexpr float subtileSize = float(TILE_SIZE) / float(GRID_SUBDIVISIONS);
        const float stepLen = subtileSize * 0.8f;
        const int   maxSteps = (int)glm::ceil(maxDistW / stepLen);

        constexpr float kPadding = 0.2f;

        glm::vec2 prevP = fromWorld;

        for (int i = 1; i <= maxSteps; ++i)
        {
            float t = (float)i / (float)maxSteps;
            glm::vec2 P = fromWorld + dirNorm * (t * maxDistW);

            if (m_gridRef->IsPointBlocked_Bucketed(P, kPadding))
            {
                if (outHit) *outHit = true;

                glm::vec2 a = prevP;
                glm::vec2 b = P;

                for (int it = 0; it < 8; ++it)
                {
                    glm::vec2 m = 0.5f * (a + b);
                    if (m_gridRef->IsPointBlocked_Bucketed(m, kPadding))
                    {
                        b = m; // still blocked, move closer to free side

                    }
                    else
                    {

                        a = m; // free, move toward blocker
                    }
                }

                // higher value moves teh fog away
                // low value fog might come too soon front of the tiles
                float whereFogStartsOffset = 0.30f;

                return a + dirNorm * whereFogStartsOffset;
            }

            prevP = P;
        }

        if (outHit) *outHit = false;
        return fromWorld + dirNorm * maxDistW;
    }


    void FogOfWar::DebugDrawVisibility(glm::vec2 playerPos)
    {
        std::vector<glm::vec2> outPtsW;
        float radiusW = 10.0f;
        int numRays = 100;

        //BuildVisibilityPolygon(playerPos, radiusW, numRays, outPtsW);
        DebugDrawVisibilityPoly(playerPos, outPtsW);
    }


    void FogOfWar::DebugDrawVisibilityPoly(const glm::vec2& originW, const std::vector<glm::vec2>& ptsW) const
    {
        if (ptsW.size() < 2) return;

        for (size_t i = 0; i < ptsW.size(); ++i)
        {
            const glm::vec2 a = ptsW[i];
            const glm::vec2 b = ptsW[(i + 1) % ptsW.size()];
            Engine::VulkanRenderer2D::DrawLine(glm::vec3(a, 0.1f), glm::vec3(b, 0.1f), glm::vec4(0, 1, 1, 1), -1);
        }

        for (size_t i = 0; i < ptsW.size(); i += 8)
        {
            DrawDebugLine(originW, ptsW[i], glm::vec4(1, 1, 0, 0.3f));
        }
    }

    void FogOfWar::DrawDebugLine(glm::vec2 from, glm::vec2 to, const glm::vec4& color) const
    {
        // EE_PROFILE_FUNCTION();
        glm::vec3 a(from, 0.1f); // slight Z offset
        glm::vec3 b(to, 0.1f);
        Engine::VulkanRenderer2D::DrawLine(a, b, color, -1);
    }


  
   

    void FogOfWar::BuildFogFan(const glm::vec2& originW, const std::vector<glm::vec2>& polyPtsW,
        std::vector<Engine::VulkanFogOfWarPipelines::FogVertex>& outFanTris)
    {
        outFanTris.clear();
        if (polyPtsW.size() < 3) return;

        outFanTris.reserve(polyPtsW.size() * 3);

        for (size_t i = 0; i < polyPtsW.size(); ++i)
        {
            const glm::vec2 a = polyPtsW[i];
            const glm::vec2 b = polyPtsW[(i + 1) % polyPtsW.size()];

            outFanTris.push_back({ originW });
            outFanTris.push_back({ a });
            outFanTris.push_back({ b });
        }
    }

    void FogOfWar::BuildFogQuad(const glm::vec2& minW, const glm::vec2& maxW,
        std::vector<Engine::VulkanFogOfWarPipelines::FogVertex>& outQuadTris)
    {
        outQuadTris.clear();
        outQuadTris.reserve(6);

        glm::vec2 p0(minW.x, minW.y);
        glm::vec2 p1(maxW.x, minW.y);
        glm::vec2 p2(maxW.x, maxW.y);
        glm::vec2 p3(minW.x, maxW.y);

        // two tris: (p0,p1,p2) and (p0,p2,p3)
        outQuadTris.push_back({ p0 }); outQuadTris.push_back({ p1 }); outQuadTris.push_back({ p2 });
        outQuadTris.push_back({ p0 }); outQuadTris.push_back({ p2 }); outQuadTris.push_back({ p3 });
    }

    void FogOfWar::BuildFogQuadNDC(std::vector<Engine::VulkanFogOfWarPipelines::FogVertex>& outQuadTris)
    {
        outQuadTris.clear();
        outQuadTris.reserve(6);

        glm::vec2 p0(-1.f, -1.f);
        glm::vec2 p1(1.f, -1.f);
        glm::vec2 p2(1.f, 1.f);
        glm::vec2 p3(-1.f, 1.f);

        outQuadTris.push_back({ p0 }); outQuadTris.push_back({ p1 }); outQuadTris.push_back({ p2 });
        outQuadTris.push_back({ p0 }); outQuadTris.push_back({ p2 }); outQuadTris.push_back({ p3 });
    }


}