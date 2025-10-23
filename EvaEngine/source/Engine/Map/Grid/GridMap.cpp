#include "pch.h"
#include "GridMap.h"
#include "TileCollisionMask.h"


#include <Engine/Scene/Components/Render/TileComponent.h>
#include <Engine/Scene/Component.h>
#include <Engine/Renderer/VulkanRenderer2D.h>
#include <glm/gtc/matrix_transform.hpp>
#include <unordered_set>q
#include "Engine/Map/Utils/IsoTileUtils.h"
#include "Engine/Scene/Scene.h"


namespace Engine
{
    void GridMap::BuildFromRegistry(Scene* scene)
    {
        m_blockedSubCells.clear();

        // --- measure iso diamond in world units (unchanged) ---
        const glm::vec2 g00 = IsoTileUtils::IsoToWorldGround({ 0,0 });
        const glm::vec2 gE = IsoTileUtils::IsoToWorldGround({ 1,-1 }); // east neighbor
        const glm::vec2 gS = IsoTileUtils::IsoToWorldGround({ 1, 1 }); // south neighbor
        const float CELL_W = std::abs(gE.x - g00.x);   // full diamond width
        const float CELL_H = std::abs(gS.y - g00.y);   // full diamond height
        if (CELL_W <= 0.f || CELL_H <= 0.f) return;

        // --- shared shaping constants ---
        constexpr int   SUBS = SUBDIVS;          // same edge segmentation you use for walls
        constexpr float SHRINK_ALONG = 0.96f;            // tiny shrink to avoid overlap
        const     float HALF_THICK_W = 0.5f * (0.22f * std::min(CELL_W, CELL_H)); // wall strip half-thickness

        // For centered props we usually want a smaller footprint:
        const     float CENTER_HALF_THICK = 0.5f * (0.12f * std::min(CELL_W, CELL_H)); // prop strip half-thickness

        enum class FootSide : uint8_t { North, East, South, West };

        auto parseSide = [](const std::string& name) -> FootSide {
            auto pos = name.find_last_of('_');
            if (pos != std::string::npos && pos + 1 < name.size()) {
                char c = (char)std::toupper(name[pos + 1]);
                if (c == 'N') return FootSide::North;
                if (c == 'E') return FootSide::East;
                if (c == 'S') return FootSide::South;
                if (c == 'W') return FootSide::West;
            }
            return FootSide::South;
            };

        auto edgeForSide = [&](FootSide side, const glm::vec2& S,
            glm::vec2& A, glm::vec2& B)
            {
                const glm::vec2 E = S + glm::vec2(+CELL_W * 0.5f, -CELL_H * 0.5f);
                const glm::vec2 W = S + glm::vec2(-CELL_W * 0.5f, -CELL_H * 0.5f);
                const glm::vec2 N = S + glm::vec2(0.0f, -CELL_H);

                switch (side) {
                case FootSide::North: A = N; B = E; break; // top-right edge
                case FootSide::East:  A = E; B = S; break; // right edge
                case FootSide::South: A = S; B = W; break; // bottom-left edge
                case FootSide::West:  A = W; B = N; break; // left edge
                }
            };

        // --- existing walls: emit strips along a tile edge ---
        auto emitEdgeSubcellsOnSide = [&](const glm::ivec2& cell, FootSide side)
            {
                const glm::vec2 S = IsoTileUtils::IsoToWorldGround(cell);

                // diamond vertices + centroid
                const glm::vec2 E = S + glm::vec2(+CELL_W * 0.5f, -CELL_H * 0.5f);
                const glm::vec2 W = S + glm::vec2(-CELL_W * 0.5f, -CELL_H * 0.5f);
                const glm::vec2 N = S + glm::vec2(0.0f, -CELL_H);
                const glm::vec2 C = (E + W + N + S) * 0.25f;

                glm::vec2 A{}, B{};
                edgeForSide(side, S, A, B);

                const glm::vec2 e = B - A;
                const float     L = glm::length(e);
                if (L <= 1e-6f) return;

                const glm::vec2 T = e / L;                // tangent along edge
                glm::vec2       Nin = glm::vec2(-T.y, T.x); // left normal

                // Make normal point inward (toward centroid)
                if (glm::dot(Nin, C - 0.5f * (A + B)) < 0.0f) Nin = -Nin;

                const float segLen = L / float(SUBS);
                const float halfAlong = 0.5f * segLen * SHRINK_ALONG;

                for (int s = 0; s < SUBS; ++s)
                {
                    const float t0 = float(s) * segLen;
                    const float t1 = float(s + 1) * segLen;
                    const float tm = 0.5f * (t0 + t1);
                    const glm::vec2 P = A + T * tm;

                    SubCellOBB obb;
                    obb.center = P + Nin * HALF_THICK_W;
                    obb.halfExtents = { halfAlong, HALF_THICK_W };
                    obb.tangent = T;
                    m_blockedSubCells.push_back(obb);
                }
            };


        auto emitCenteredStrip = [&](const glm::ivec2& cell,
            float widthFrac,
            float thickFrac,
            float yNudgePx /* NEW: positive pushes DOWN if +Y is down */)
            {
                const glm::vec2 S = IsoTileUtils::IsoToWorldGround(cell);
                const glm::vec2 E = S + glm::vec2(+CELL_W * 0.5f, -CELL_H * 0.5f);
                const glm::vec2 W = S + glm::vec2(-CELL_W * 0.5f, -CELL_H * 0.5f);
                const glm::vec2 N = S + glm::vec2(0.0f, -CELL_H);
                const glm::vec2 C = (E + W + N + S) * 0.25f;

                const glm::vec2 T = glm::normalize(E - W);
                const float halfAlong = 0.5f * widthFrac * 0.5f * CELL_W;
                const float halfThick = 0.5f * thickFrac * std::min(CELL_W, CELL_H);

                SubCellOBB obb;
                obb.center = C + glm::vec2(0.0f, PxToWorld(yNudgePx)); // <<< vertical nudge here
                obb.halfExtents = { halfAlong, halfThick };
                obb.tangent = T;
                m_blockedSubCells.push_back(obb);
            };

        auto emitCenteredDiscApprox = [&](const glm::ivec2& cell, float radiusFrac /*~0.18f*/)
            {
                const glm::vec2 S = IsoTileUtils::IsoToWorldGround(cell);
                const glm::vec2 E = S + glm::vec2(+CELL_W * 0.5f, -CELL_H * 0.5f);
                const glm::vec2 W = S + glm::vec2(-CELL_W * 0.5f, -CELL_H * 0.5f);
                const glm::vec2 N = S + glm::vec2(0.0f, -CELL_H);
                const glm::vec2 C = (E + W + N + S) * 0.25f;

                const float R = radiusFrac * 0.5f * std::min(CELL_W, CELL_H);

                // Two orthogonal strips crossing at C
                auto pushStrip = [&](const glm::vec2& T)
                    {
                        SubCellOBB obb;
                        obb.center = C;
                        obb.tangent = glm::normalize(T);
                        obb.halfExtents = { R, 0.5f * R };
                        m_blockedSubCells.push_back(obb);
                    };

                pushStrip(E - W);     // “horizontal”
                pushStrip(N - S);     // “vertical”
            };

        // Per-side anchor correction used for walls (leave as-is)
        auto sideIsoOffset = [](FootSide s) -> glm::ivec2 {
            switch (s) {
            case FootSide::North: return { +1, +1 };
            case FootSide::South: return { +1, +1 };
            case FootSide::East:  return { 0, +1 };
            case FootSide::West:  return { +2, +1 };
            }
            return { 0,0 };
            };

        // --- Walk tiles and emit collision sub-cells ---
        auto view = scene->GetRegistry().view<TileComponent, TransformComponent>();
        for (auto e : view)
        {
            const auto& tc = view.get<TileComponent>(e);
            const auto& tr = view.get<TransformComponent>(e);

            for (const auto& t : tc.tiles)
            {
                // Tile ground = entity anchor + local placement
                const glm::vec2 ground = glm::vec2(tr.Translation) + t.position;
                glm::ivec2 cell = IsoTileUtils::WorldToIsoCell(ground);

                if (t.Category == eTileCategory::Buildings)
                {
                    FootSide side = parseSide(t.name);
                    cell += sideIsoOffset(side);           // edge anchoring like your walls
                    emitEdgeSubcellsOnSide(cell, side);
                }
                
                else if (t.Category == eTileCategory::dynamicObjects)
                {
                    // Defaults that work well for lamps/signs; make these data-driven later:
                    // If you already computed a per-tile foot width in ExtractPixelsFromTilePallette,
                    // map it to a fraction and use it here instead of the constants below.
                    constexpr float kDefaultWidthFrac = 0.35f; // 35% of tile width
                    constexpr float kDefaultThickFrac = 0.12f; // 12% of min(CELL_W,H)
                    constexpr float yNudgePx = 90.f; // 12% of min(CELL_W,H)
                    constexpr bool  kUseDiscForPosts = false; // flip true for poles/trees

                    if (kUseDiscForPosts)
                    {
                        emitCenteredDiscApprox(cell, 0.18f);
                    }
                    else
                    {
                        emitCenteredStrip(cell, kDefaultWidthFrac, kDefaultThickFrac, yNudgePx);
                    }
                }
                
                
            }
        }
    }




    void GridMap::MarkBlockedSubtilesFromTexture(
        const glm::vec2& worldPosition, // tile position in world units (e.g., (5, 10))
        const std::vector<uint8_t>& textureData,
        uint32_t textureWidth, uint32_t textureHeight)
    {

        for (uint32_t y = 0; y < textureHeight; ++y)
        {
            for (uint32_t x = 0; x < textureWidth; ++x)
            {
                size_t index = (y * textureWidth + x) * 4;
                uint8_t alpha = textureData[index + 3];
                if (alpha < 10) continue; // skip transparent pixels

                // Pixel position in fractional tile units
                glm::vec2 pixelOffsetInTile = glm::vec2(float(x) / float(TILE_PIXEL_WIDTH), float(y) / float(TILE_PIXEL_WIDTH));

                // Total subtile world position
                glm::vec2 subtileWorldPos = worldPosition + pixelOffsetInTile;

                // Convert to subtile grid coordinate (integers)
                glm::ivec2 subtileGridCoord = glm::floor(subtileWorldPos * float(GRID_SUBDIVISIONS));

              //  m_blockedTiles.insert(subtileGridCoord);
            }
        }
    }





	bool GridMap::IsBlocked(glm::ivec2 worldTileCoords) const
	{
        EE_PROFILE_FUNCTION();
		//return m_blockedTiles.find(worldTileCoords) != m_blockedTiles.end();
        return false;
	}



    bool GridMap::HasLineOfSight(glm::vec2 fromWorld, glm::vec2 toWorld, bool debugDraw)
    {
        EE_PROFILE_FUNCTION();

        constexpr float subtileSize = float(TILE_SIZE) / float(GRID_SUBDIVISIONS);

        // Convert world position to subtile coordinates:
        glm::ivec2 from = glm::floor(fromWorld / subtileSize);
        glm::ivec2 to = glm::floor(toWorld / subtileSize);

        int x0 = from.x;
        int y0 = from.y;
        int x1 = to.x;
        int y1 = to.y;

        int dx = abs(x1 - x0);
        int dy = abs(y1 - y0);
        int sx = x0 < x1 ? 1 : -1;
        int sy = y0 < y1 ? 1 : -1;
        int err = dx - dy;

        bool isFirstTile = true;

        while (true)
        {
            glm::ivec2 subtileCoord = { x0, y0 };

           if (!isFirstTile && m_blockedTiles.find(subtileCoord) != m_blockedTiles.end())
            {
                if (debugDraw)
                {
                    glm::vec2 subtileCenter = glm::vec2(subtileCoord) * subtileSize + glm::vec2(subtileSize * 0.5f);
                    glm::mat4 model = glm::translate(glm::mat4(1.0f), glm::vec3(subtileCenter, 0.0f)) *
                        glm::scale(glm::mat4(1.0f), glm::vec3(subtileSize));
                    Engine::VulkanRenderer2D::DrawLineRect(model, glm::vec4(1, 0, 0, 0.3f), -1.0f);
                    DrawDebugLine(fromWorld, toWorld, glm::vec4(1, 0, 0, 1));
                }
                return false;
            }

            if (debugDraw)
            {
                glm::vec2 subtileCenter = glm::vec2(subtileCoord) * subtileSize + glm::vec2(subtileSize * 0.5f);
                glm::mat4 model = glm::translate(glm::mat4(1.0f), glm::vec3(subtileCenter, 0.0f)) *
                    glm::scale(glm::mat4(1.0f), glm::vec3(subtileSize));
                Engine::VulkanRenderer2D::DrawLineRect(model, glm::vec4(1, 1, 0, 0.2f), -1.0f);
            }

            if (x0 == x1 && y0 == y1)
                break;

            int e2 = 2 * err;
            if (e2 > -dy)
            {
                err -= dy;
                x0 += sx;
            }
            if (e2 < dx)
            {
                err += dx;
                y0 += sy;
            }

            isFirstTile = false;
        }

        if (debugDraw)
        {
            glm::vec2 subtileCenter = glm::vec2(to) * subtileSize + glm::vec2(subtileSize * 0.5f);
            glm::mat4 model = glm::translate(glm::mat4(1.0f), glm::vec3(subtileCenter, 0.0f)) *
                glm::scale(glm::mat4(1.0f), glm::vec3(subtileSize));
            Engine::VulkanRenderer2D::DrawLineRect(model, glm::vec4(0, 1, 1, 0.4f), -1.0f);
            DrawDebugLine(fromWorld, toWorld, glm::vec4(0, 1, 0, 1));
        }

        return true;
    }
    void GridMap::UpdateTiles()
    {
        EE_PROFILE_FUNCTION();

        const auto& rects = Engine::TileBlockedMaskCPU::DirtRects;
        if (rects.empty() || m_blockedSubCells.empty()) return;

        m_debugRects.clear();

        // ---- Tunables ----
        static constexpr float DIRTY_CELLS_PER_TILE = 16.0f;
        static constexpr float INSET_FRAC_PER_AXIS = 0.50f; // shrink dirty rect by N % of 1 cell on each side
        static constexpr float MIN_OVERLAP_FRAC = 0.45f; // require >=N% of subcell AABB overlapped

        const glm::vec2 tileSizeW = glm::vec2(float(TILE_SIZE));
        const glm::vec2 cellSizeW = tileSizeW / DIRTY_CELLS_PER_TILE;

        // >>> Your desired physical shift of the DESTRUCTION rects <<<
        // If your Y is up, use +; if screen-space Y-down, flip the sign.
        const glm::vec2 testBias = glm::vec2(0.0f, 0.3f * float(TILE_SIZE));

        std::vector<uint8_t> kill(m_blockedSubCells.size(), 0);

        auto aabbOverlapArea = [](const glm::vec2& a0, const glm::vec2& a1,
            const glm::vec2& b0, const glm::vec2& b1) -> float {
                float dx = std::max(0.f, std::min(a1.x, b1.x) - std::max(a0.x, b0.x));
                float dy = std::max(0.f, std::min(a1.y, b1.y) - std::max(a0.y, b0.y));
                return dx * dy;
            };
        auto aabbArea = [](const glm::vec2& m, const glm::vec2& M) -> float {
            return std::max(0.f, M.x - m.x) * std::max(0.f, M.y - m.y);
            };

        for (const auto& r : rects)
        {
            const glm::vec2 tileMinW = r.topLeft;

            // Rect in WORLD (unbiased first)
            glm::vec2 wmin = tileMinW + glm::vec2(r.minCell) * cellSizeW;
            glm::vec2 wmax = tileMinW + glm::vec2(r.maxCell) * cellSizeW;
            if (wmin.x > wmax.x) std::swap(wmin.x, wmax.x);
            if (wmin.y > wmax.y) std::swap(wmin.y, wmax.y);

            // Inset (guard grazing)
            const glm::vec2 inset = INSET_FRAC_PER_AXIS * cellSizeW;
            glm::vec2 wminS = wmin + inset;
            glm::vec2 wmaxS = wmax - inset;
            wminS = glm::min(wminS, wmaxS);

            // >>> Apply the SAME bias to the rect for testing & drawing <<<
            const glm::vec2 rMinT = wminS + testBias;
            const glm::vec2 rMaxT = wmaxS + testBias;

            // Debug draw at the true (biased) physical location
            PushDirtyDebugRectWorld(rMinT, rMaxT, { 0.2f, 0.5f, 1.0f, 1.0f });

            for (size_t i = 0; i < m_blockedSubCells.size(); ++i)
            {
                if (kill[i]) continue;

                glm::vec2 obbMinW, obbMaxW;
                GridUtils::OBB_ComputeAABB(m_blockedSubCells[i], obbMinW, obbMaxW);
                if (obbMinW.x > obbMaxW.x) std::swap(obbMinW.x, obbMaxW.x);
                if (obbMinW.y > obbMaxW.y) std::swap(obbMinW.y, obbMaxW.y);

                // >>> Apply the SAME bias to the SUBCELL AABB *for the test only* <<<
                // (This does not change stored subcells; it’s just local variables.)
                const glm::vec2 sMinT = obbMinW + testBias;
                const glm::vec2 sMaxT = obbMaxW + testBias;

                // Quick separation with small epsilon
                constexpr float eps = 1e-6f;
                const bool sep =
                    (sMaxT.x < rMinT.x + eps) || (sMinT.x > rMaxT.x - eps) ||
                    (sMaxT.y < rMinT.y + eps) || (sMinT.y > rMaxT.y - eps);
                if (sep) continue;

                // Area guard
                const float ovA = aabbOverlapArea(sMinT, sMaxT, rMinT, rMaxT);
                const float obA = aabbArea(sMinT, sMaxT);
                if (!(obA > 0.f) || (ovA / obA) < MIN_OVERLAP_FRAC) continue;

                kill[i] = 1;
            }
        }

        // Compact
        size_t w = 0;
        for (size_t i = 0; i < m_blockedSubCells.size(); ++i)
            if (!kill[i]) m_blockedSubCells[w++] = m_blockedSubCells[i];
        m_blockedSubCells.resize(w);
    }







	void GridMap::DrawDebugLine(glm::vec2 from, glm::vec2 to, const glm::vec4& color)
	{
        EE_PROFILE_FUNCTION();
		glm::vec3 a(from, 0.1f); // slight Z offset
		glm::vec3 b(to, 0.1f);
		Engine::VulkanRenderer2D::DrawLine(a, b, color, -1);
	}

    void GridMap::DrawDebugBlockedTiles() const
    {
        EE_PROFILE_FUNCTION();

        for (const auto& r : m_debugRects)
        {
            DrawAABB_LineRect(r.minW, r.maxW, r.color);
        }

        const glm::vec4 outline = { 0.2f, 1.0f, 0.3f, 0.95f };
        const glm::vec4 fill = { 0.2f, 1.0f, 0.3f, 0.35f };
        const float z = 0.002f;



        for (const SubCellOBB& obb : m_blockedSubCells)
        {
            // Local axes: T along the edge, N is the perpendicular.
            glm::vec2 T = obb.tangent;
            // Try LEFT normal first:
            glm::vec2 N = glm::vec2(-T.y, T.x);
            // If the rectangle appears mirrored/flipped in your engine, swap to RIGHT normal:
            // glm::vec2 N = glm::vec2(T.y, -T.x);

            // Build a 4x4 basis with columns [T N Z translate], then scale in local space
            glm::mat4 M(1.0f);
            // column 0 = T
            M[0] = glm::vec4(T.x, T.y, 0.0f, 0.0f);
            // column 1 = N
            M[1] = glm::vec4(N.x, N.y, 0.0f, 0.0f);
            // column 2 = Z
            M[2] = glm::vec4(0.0f, 0.0f, 1.0f, 0.0f);
            // column 3 = translation
            M[3] = glm::vec4(obb.center.x, obb.center.y, z, 1.0f);

            // Scale along local T/N axes (double half-extents to get full size)
            glm::mat4 S = glm::scale(glm::mat4(1.0f),
                glm::vec3(obb.halfExtents.x * 2.0f,
                    obb.halfExtents.y * 2.0f, 1.0f));

            glm::mat4 model = M * S;

            
            Engine::VulkanRenderer2D::DrawLineRect(model, outline, -1.0f);

            // (Optional) draw the edge centerline to visually confirm orientation:
            // glm::vec2 p0 = obb.center - T * obb.halfExtents.x;
            // glm::vec2 p1 = obb.center + T * obb.halfExtents.x;
            // Engine::VulkanRenderer2D::DrawLine(p0, p1, outline);
        }
    }

    inline void GridMap::DrawAABB_LineRect(const glm::vec2& wmin,
        const glm::vec2& wmax,
        const glm::vec4& color)
    {
        glm::vec2 a = wmin, b = wmax;
        if (a.x > b.x) std::swap(a.x, b.x);
        if (a.y > b.y) std::swap(a.y, b.y);

        const glm::vec2 size = glm::max(b - a, glm::vec2(1e-6f));
        const glm::vec2 center = 0.5f * (a + b);

        // For centered unit quad: translate to center, then scale
        const glm::mat4 M = glm::translate(glm::mat4(1.f), glm::vec3(center, 0.f))
            * glm::scale(glm::mat4(1.f), glm::vec3(size, 1.f));

        Engine::VulkanRenderer2D::DrawLineRect(M, color);
    }

    inline void GridMap::PushDirtyDebugRectWorld(const glm::vec2& wmin, const glm::vec2& wmax, const glm::vec4& color)
    {

        m_debugRects.push_back({ wmin, wmax, color });
    }

}
