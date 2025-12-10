#include "pch.h"
#include "GridUtils.h"
#include "Engine/Map/Utils/IsoTileUtils.h"

namespace Engine
{
   
    glm::vec2 GridUtils::PerpCCW(const glm::vec2& v)
    {
        return { -v.y, v.x };
    }

    bool GridUtils::OBBvsAABB(const SubCellOBB& obb, const glm::vec2& bmin, const glm::vec2& bmax)
    {
        glm::vec2 T = obb.tangent;
        float tl = glm::length(T);
        if (tl < 1e-8f) T = { 1,0 }; else T /= tl;
        glm::vec2 N = PerpCCW(T);
        const float hx = obb.halfExtents.x, hy = obb.halfExtents.y;

        // 1) Project AABB corners onto OBB axes (T,N) and test overlap with [-hx,hx] & [-hy,hy]
        glm::vec2 ac[4] = { {bmin.x,bmin.y},{bmax.x,bmin.y},{bmax.x,bmax.y},{bmin.x,bmax.y} };
        float minU = +FLT_MAX, maxU = -FLT_MAX, minV = +FLT_MAX, maxV = -FLT_MAX;
        for (int i = 0; i < 4; ++i)
        {
            glm::vec2 d = ac[i] - obb.center;
            float u = glm::dot(d, T);
            float v = glm::dot(d, N);
            minU = std::min(minU, u); maxU = std::max(maxU, u);
            minV = std::min(minV, v); maxV = std::max(maxV, v);
        }
        if (maxU < -hx || minU > hx) return false; // separated on T
        if (maxV < -hy || minV > hy) return false; // separated on N

        // 2) Project OBB corners onto world X and Y, compare with [bmin,bmax]
        glm::vec2 c = obb.center;
        glm::vec2 oc[4] = {
            c - T * hx - N * hy, c + T * hx - N * hy,
            c + T * hx + N * hy, c - T * hx + N * hy
        };
        float ominX = +FLT_MAX, omaxX = -FLT_MAX, ominY = +FLT_MAX, omaxY = -FLT_MAX;
        for (int i = 0; i < 4; ++i)
        {
            ominX = std::min(ominX, oc[i].x); omaxX = std::max(omaxX, oc[i].x);
            ominY = std::min(ominY, oc[i].y); omaxY = std::max(omaxY, oc[i].y);
        }
        if (omaxX < bmin.x || ominX > bmax.x) return false; // separated on world X
        if (omaxY < bmin.y || ominY > bmax.y) return false; // separated on world Y

        return true; // overlaps on all 4 SAT axes
    }

    void GridUtils::SubtileAABB(const glm::ivec2& gs, float subtileSize,
        glm::vec2& bmin, glm::vec2& bmax)
    {
        bmin = glm::vec2(gs) * subtileSize;
        bmax = bmin + glm::vec2(subtileSize);
    }

    bool GridUtils::AABBoverlap(const glm::vec2& amin, const glm::vec2& amax,
        const glm::vec2& bmin, const glm::vec2& bmax)
    {
        return !(amax.x < bmin.x || amin.x > bmax.x ||
            amax.y < bmin.y || amin.y > bmax.y);
    }

    void GridUtils::OBB_ComputeAABB(const SubCellOBB& obb, glm::vec2& outMin, glm::vec2& outMax)
    {
        glm::vec2 t = obb.tangent;
        float tl = glm::length(t);
        if (tl > 0.0f) t /= tl;             // normalize if needed
        glm::vec2 n = glm::vec2(-t.y, t.x); // perpendicular

        // world-projected half extents on x and y axes
        glm::vec2 ex = glm::abs(glm::vec2(t.x, n.x)) * glm::vec2(obb.halfExtents.x, obb.halfExtents.y);
        glm::vec2 ey = glm::abs(glm::vec2(t.y, n.y)) * glm::vec2(obb.halfExtents.x, obb.halfExtents.y);
        glm::vec2 hw = ex + ey;

        outMin = obb.center - hw;
        outMax = obb.center + hw;
    }


    bool GridUtils::PointInSubCellOBB(const glm::vec2& P, const SubCellOBB& obb)
    {
        const glm::vec2 T = obb.tangent;
        const glm::vec2 N = glm::vec2(-T.y, T.x);

        glm::vec2 d = P - obb.center;

        float x = glm::dot(d, T);
        float y = glm::dot(d, N);

        return std::abs(x) <= obb.halfExtents.x &&
            std::abs(y) <= obb.halfExtents.y;
    }

    bool GridUtils::PointInSubCellOBB_Padded(const glm::vec2& P, const SubCellOBB& obb, float padding)
    {
        const glm::vec2 T = obb.tangent;
        const glm::vec2 N = glm::vec2(-T.y, T.x);

        glm::vec2 d = P - obb.center;

        float x = glm::dot(d, T);
        float y = glm::dot(d, N);

        float hx = obb.halfExtents.x + padding;
        float hy = obb.halfExtents.y + padding;

        return std::abs(x) <= hx &&
            std::abs(y) <= hy;
    }


    bool GridUtils::OBB_IntersectsCircle(const SubCellOBB& obb, const glm::vec2& C, float R)
    {
        glm::vec2 t = obb.tangent;
        float tl = glm::length(t);
        if (tl > 0.0f) t /= tl;
        glm::vec2 n = glm::vec2(-t.y, t.x);

        // circle center in OBB-local coords
        glm::vec2 rel = C - obb.center;
        float x = glm::dot(rel, t);
        float y = glm::dot(rel, n);

        // closest point on OBB to the circle center
        float cx = glm::clamp(x, -obb.halfExtents.x, obb.halfExtents.x);
        float cy = glm::clamp(y, -obb.halfExtents.y, obb.halfExtents.y);

        float dx = x - cx;
        float dy = y - cy;
        return (dx * dx + dy * dy) <= (R * R);
    }

}