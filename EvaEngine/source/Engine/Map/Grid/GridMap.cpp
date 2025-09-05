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

        // --- measure iso diamond in your world units ---
        const glm::vec2 g00 = IsoTileUtils::IsoToWorldGround({ 0, 0 });
        const glm::vec2 gE = IsoTileUtils::IsoToWorldGround({ 1, -1 }); // east neighbor
        const glm::vec2 gS = IsoTileUtils::IsoToWorldGround({ 1,  1 }); // south neighbor
        const float CELL_W = std::abs(gE.x - g00.x);  // full diamond width
        const float CELL_H = std::abs(gS.y - g00.y);  // full diamond height
        if (CELL_W <= 0.f || CELL_H <= 0.f) return;

        // sub-cell layout
        constexpr int   SUBS = SUBDIVS;         // 3 segments along the edge
        constexpr float SHRINK_ALONG = 0.96f;    // tiny shrink along edge to avoid overlap
        const     float HALF_THICK = 0.5f * (0.22f * std::min(CELL_W, CELL_H)); // strip thickness/2

        enum class FootSide : uint8_t { North, East, South, West };

        // parse side from tile name ("..._N", "..._E", "..._S", "..._W")
        auto parseSide = [](const std::string& name) -> FootSide {
            auto pos = name.find_last_of('_');
            if (pos != std::string::npos && pos + 1 < name.size()) {
                char c = (char)std::toupper(name[pos + 1]);
                if (c == 'N') return FootSide::North;
                if (c == 'E') return FootSide::East;
                if (c == 'S') return FootSide::South;
                if (c == 'W') return FootSide::West;
            }
            return FootSide::South; // sensible default
            };

        // pick the edge *segment* for each side (90° CW vs your previous mapping)
        // Diamond vertices from ground S (south tip):
        //   N(0,-CELL_H), E(+W/2,-H/2), S(0,0), W(-W/2,-H/2)
        // Edges: N?E, E?S, S?W, W?N (clockwise)
        auto edgeForSide = [&](FootSide side, const glm::vec2& S,
            glm::vec2& A, glm::vec2& B)
            {
                const glm::vec2 E = S + glm::vec2(+CELL_W * 0.5f, -CELL_H * 0.5f);
                const glm::vec2 W = S + glm::vec2(-CELL_W * 0.5f, -CELL_H * 0.5f);
                const glm::vec2 N = S + glm::vec2(0.0f, -CELL_H);

                switch (side) {
                case FootSide::North: A = N; B = E; break; // use top-right edge
                case FootSide::East:  A = E; B = S; break; // right edge
                case FootSide::South: A = S; B = W; break; // bottom-left edge
                case FootSide::West:  A = W; B = N; break; // left edge
                }
            };

        auto emitSubcellsOnEdge = [&](const glm::ivec2& cell, FootSide side)
            {
                const glm::vec2 S = IsoTileUtils::IsoToWorldGround(cell);

                // vertices (for centroid / inward normal)
                const glm::vec2 E = S + glm::vec2(+CELL_W * 0.5f, -CELL_H * 0.5f);
                const glm::vec2 W = S + glm::vec2(-CELL_W * 0.5f, -CELL_H * 0.5f);
                const glm::vec2 N = S + glm::vec2(0.0f, -CELL_H);
                const glm::vec2 C = (E + W + N + S) * 0.25f; // diamond centroid

                glm::vec2 A{}, B{};
                edgeForSide(side, S, A, B);

                const glm::vec2 e = B - A;
                const float     L = glm::length(e);
                if (L <= 1e-6f) return;

                const glm::vec2 T = e / L;               // tangent along the edge
                glm::vec2       Nin = glm::vec2(-T.y, T.x); // left normal

                // make normal point inward (towards centroid)
                const glm::vec2 midEdge = 0.5f * (A + B);
                if (glm::dot(Nin, C - midEdge) < 0.0f) Nin = -Nin;

                const float segLen = L / float(SUBS);
                const float halfAlong = 0.5f * segLen * SHRINK_ALONG;

                for (int s = 0; s < SUBS; ++s)
                {
                    const float t0 = float(s) * segLen;
                    const float t1 = float(s + 1) * segLen;
                    const float tm = 0.5f * (t0 + t1);
                    const glm::vec2 P = A + T * tm;                  // mid point on edge

                    SubCellOBB obb;
                    obb.center = P + Nin * HALF_THICK;          // push inward
                    obb.halfExtents = { halfAlong, HALF_THICK };
                    obb.tangent = T;                             // orientation
                    m_blockedSubCells.push_back(obb);
                }
            };
        auto sideIsoOffset = [](FootSide s) -> glm::ivec2 {
            switch (s) {
            case FootSide::North: return { +1, +1 };  // DR one cell (matches what fixed N/S for you)
            case FootSide::South: return { +1, +1 };  // DR one cell
            case FootSide::East:  return { 0, +1 };  // EAST neighbor in iso (u+1, v-1)
            case FootSide::West:  return { +2, +1 };  // WEST neighbor in iso (u-1, v+1)
            }
            return { 0,0 };
            };

        // walk tiles and emit sub-cells
        auto view = scene->GetRegistry().view<TileComponent, TransformComponent>();
        for (auto e : view)
        {
            const auto& tc = view.get<TileComponent>(e);
            const auto& tr = view.get<TransformComponent>(e);

            for (const auto& t : tc.tiles)
            {
                if (t.Category != eTileCategory::Buildings) continue;

                // ground = entity anchor + local world delta you stored
                const glm::vec2  ground = glm::vec2(tr.Translation) + t.position;
                glm::ivec2       cell = IsoTileUtils::WorldToIsoCell(ground);

                FootSide side = parseSide(t.name);

                // <<< per-side anchor correction >>>
                cell += sideIsoOffset(side);

                emitSubcellsOnEdge(cell, side);
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

	void GridMap::Clear()
	{
		//m_blockedTiles.clear();
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

   glm::vec2 GridMap::PerpCCW(const glm::vec2& v)
   {
       return { -v.y, v.x };
   }

    // SAT: OBB vs axis-aligned AABB
    bool GridMap::OBBvsAABB(const SubCellOBB& obb, const glm::vec2& bmin, const glm::vec2& bmax)
    {
        // OBB basis
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

    void GridMap::SubtileAABB(const glm::ivec2& gs, float subtileSize,
        glm::vec2& bmin, glm::vec2& bmax)
    {
        bmin = glm::vec2(gs) * subtileSize;
        bmax = bmin + glm::vec2(subtileSize);
    }
    
    void GridMap::UpdateTiles(const glm::ivec2& minOrigin)
    {
        EE_PROFILE_FUNCTION();

        const auto& mask = TileBlockedMaskCPU::CachedGPUMask;
        if (mask.empty()) return;

      
        const uint32_t spt = GRID_SUBDIVISIONS; // SUBTILES_PER_TILE
        const uint32_t tilesPerRow = CHUNK_SIZE * CHUNK_GRID_WIDTH;
        const uint32_t tilesPerCol = CHUNK_SIZE * CHUNK_GRID_WIDTH;
        const uint32_t subtilesPerRow = tilesPerRow * spt;
        (void)tilesPerCol; // rows count not needed explicitly

        const float tileWorld = float(TILE_SIZE);           
        const float subtileSize = tileWorld / float(spt);

        // Mark OBBs to remove
        std::vector<uint8_t> kill(m_blockedSubCells.size(), 0);

        // Walk GPU mask: only care about DESTROYED bit
        for (uint32_t i = 0; i < mask.size(); ++i)
        {
            const uint32_t bits = mask[i];
            if ((bits & MASK_DESTROYED) == 0u) continue;

            // Subtile inside the 3x3 window
            const glm::ivec2 rel = { int(i % subtilesPerRow), int(i / subtilesPerRow) };

            // Convert to global AA-subtile coord (matches shader write)
            const glm::ivec2 gs = minOrigin * int(CHUNK_SIZE * spt) + rel;

            // World AABB of that subtile
            glm::vec2 bmin, bmax;
            SubtileAABB(gs, subtileSize, bmin, bmax);

            // Test against every live subcell; mark for removal on overlap
            for (size_t k = 0; k < m_blockedSubCells.size(); ++k)
            {
                if (kill[k])
                {
                    continue;
                }
                if (OBBvsAABB(m_blockedSubCells[k], bmin, bmax))
                {
                    kill[k] = 1;
                }
            }
        }

        // Compact m_blockedSubCells by removing killed entries (stable order not required)
        size_t w = 0;
        for (size_t k = 0; k < m_blockedSubCells.size(); ++k)
        {
            if (!kill[k])
            {

                m_blockedSubCells[w++] = m_blockedSubCells[k];
            }

        }
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

            // Optional fill:
            // Engine::VulkanRenderer2D::DrawQuad(model, fill);
            Engine::VulkanRenderer2D::DrawLineRect(model, outline, -1.0f);

            // (Optional) draw the edge centerline to visually confirm orientation:
            // glm::vec2 p0 = obb.center - T * obb.halfExtents.x;
            // glm::vec2 p1 = obb.center + T * obb.halfExtents.x;
            // Engine::VulkanRenderer2D::DrawLine(p0, p1, outline);
        }
    }


}
