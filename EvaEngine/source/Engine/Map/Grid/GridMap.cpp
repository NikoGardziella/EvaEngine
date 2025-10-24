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
#include <Engine/Renderer/Utils/DeltaBitReader.h>


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

        std::vector<DirtyTileRuntime>& Tiles = Engine::TileBlockedMaskCPU::DirtyTileRuntime;
        if (Tiles.empty() || m_blockedSubCells.empty())
            return;

        static constexpr float MIN_DEAD_FRAC = 0.45f;  // >=N% of overlapped pixels dead -> kill subcell
        static constexpr float INSET_FRAC = 0.50f;  // inset AABB by 0.5 px (per axis) to avoid grazing

        const glm::vec2 tileSizeW = glm::vec2(float(TILE_SIZE), float(TILE_SIZE));


        const int   TILE_PIX_W = TILE_PIXEL_WIDTH;   // e.g. 128
        const int   TILE_PIX_H = TILE_PIXEL_HEIGHT;  // e.g. 256
        const float pxW = tileSizeW.x / float(TILE_PIX_W);
        const float pxH = tileSizeW.y / float(TILE_PIX_H);
        

        std::vector<uint8_t> kill(m_blockedSubCells.size(), 0);

        // For each tile that has a delta/alive mask, test all subcells against it
        for (const auto& tr : Tiles)
        {
            if (tr.aliveBits.empty())
                continue;

            const glm::vec2 tileMinW = tr.topLeft;
            const glm::vec2 tileMaxW = tileMinW + tileSizeW;

            
            auto isDead = [&](int x, int y) -> bool {
                if ((unsigned)x >= (unsigned)TILE_PIX_W || (unsigned)y >= (unsigned)TILE_PIX_H) return false;
                const int idx = y * TILE_PIX_W + x;
                const int byteIdx = idx >> 3;
                const int bit = idx & 7;
                return (tr.aliveBits[byteIdx] & (1u << bit)) == 0;
                };

            // Test each subcell AABB against this tile's dead mask
            for (size_t i = 0; i < m_blockedSubCells.size(); ++i)
            {
                if (kill[i]) continue;

                glm::vec2 scMinW, scMaxW;
                GridUtils::OBB_ComputeAABB(m_blockedSubCells[i], scMinW, scMaxW);
                if (scMinW.x > scMaxW.x) std::swap(scMinW.x, scMaxW.x);
                if (scMinW.y > scMaxW.y) std::swap(scMinW.y, scMaxW.y);

                uint32_t anotherGeniusOffset = -0.5f;
                scMinW.y += anotherGeniusOffset;

                // Early reject: no overlap with this tile
                if (scMaxW.x <= tileMinW.x || scMinW.x >= tileMaxW.x ||
                    scMaxW.y <= tileMinW.y || scMinW.y >= tileMaxW.y)
                    continue;

                // Overlap in world with tile bounds
                glm::vec2 ovMinW = glm::max(scMinW, tileMinW);
                glm::vec2 ovMaxW = glm::min(scMaxW, tileMaxW);
                if (ovMinW.x >= ovMaxW.x || ovMinW.y >= ovMaxW.y) continue;

                // Inset by fraction of a pixel to avoid grazing
                const glm::vec2 insetW(INSET_FRAC * pxW, INSET_FRAC * pxH);
                ovMinW += insetW;
                ovMaxW -= insetW;
                if (ovMinW.x >= ovMaxW.x || ovMinW.y >= ovMaxW.y) continue;

                // Convert world overlap to tile pixel rect [x0,x1)×[y0,y1)
                int x0 = (int)std::floor((ovMinW.x - tileMinW.x) / pxW);
                int y0 = (int)std::floor((ovMinW.y - tileMinW.y) / pxH);
                int x1 = (int)std::ceil((ovMaxW.x - tileMinW.x) / pxW);
                int y1 = (int)std::ceil((ovMaxW.y - tileMinW.y) / pxH);

                x0 = std::clamp(x0, 0, TILE_PIX_W);  x1 = std::clamp(x1, 0, TILE_PIX_W);
                y0 = std::clamp(y0, 0, TILE_PIX_H);  y1 = std::clamp(y1, 0, TILE_PIX_H);
                if (x1 <= x0 || y1 <= y0) continue;

                // Vote to kill if enough dead pixels in the overlapped region
                const int total = (x1 - x0) * (y1 - y0);
                const int needDead = std::max(1, int(std::ceil(MIN_DEAD_FRAC * float(total))));
                int deadCount = 0;

                for (int y = y0; y < y1 && deadCount < needDead; ++y) {
                    for (int x = x0; x < x1 && deadCount < needDead; ++x) {
                        if (isDead(x, y)) ++deadCount;
                    }
                }

                if (deadCount >= needDead) {
                    kill[i] = 1;
                }
            }
        }

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
