#include "pch.h"
#include "GridMap.h"
#include "TileCollisionMask.h"


#include <Engine/Scene/Components/Render/TileComponent.h>
#include <Engine/Scene/Component.h>
#include <Engine/Renderer/Renderer2D/VulkanRenderer2D.h>
#include <glm/gtc/matrix_transform.hpp>
#include <unordered_set>
#include "Engine/Map/Utils/IsoTileUtils.h"
#include "Engine/Scene/Scene.h"
#include "Engine/Map/Grid/TileCollisionMask.h"
#include <Engine/Core/Config.h>


namespace Engine
{

    inline static uint64_t PackXY(int x, int y) {
        return (uint64_t)(uint32_t)x | ((uint64_t)(uint32_t)y << 32);
    }

     
    void GridMap::BuildFromRegistry(Scene* scene)
    {
        EE_PROFILE_FUNCTION();

        m_blockedSubCells.clear();
        m_cellToSubcells.clear();

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
        auto emitEdgeSubcellsOnSide = [&](const glm::ivec2& cell, FootSide side, uint32_t slot)
            {
                const glm::vec2 S = IsoTileUtils::IsoToWorldGround(cell);

                // diamond vertices + centroid
                const glm::vec2 E = S + glm::vec2(+CELL_W * 0.5f, -CELL_H * 0.5f);
                const glm::vec2 W = S + glm::vec2(-CELL_W * 0.5f, -CELL_H * 0.5f);
                const glm::vec2 N = S + glm::vec2(0.0f, -CELL_H);
            

                const glm::vec2 C = (E + W + N + S);

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
                    obb.TileSlot = slot;
                    m_blockedSubCells.push_back(obb);

                    
                }
            };


        auto emitCenteredStrip = [&](const glm::ivec2& cell,
            float widthFrac,
            float thickFrac,
            float yNudgePx /* NEW: positive pushes DOWN if +Y is down */,
            uint32_t slot)
            {
                const glm::vec2 S = IsoTileUtils::IsoToWorldGround(cell);
                const glm::vec2 E = S + glm::vec2(+CELL_W * 0.5f, -CELL_H * 0.5f);
                const glm::vec2 W = S + glm::vec2(-CELL_W * 0.5f, -CELL_H * 0.5f);
                const glm::vec2 N = S + glm::vec2(0.0f, -CELL_H);
                const glm::vec2 C = (E + W + N + S) * 0.25f;;

                const glm::vec2 T = glm::normalize(E - W);
                const float halfAlong = 0.5f * widthFrac * 0.5f * CELL_W;
                const float halfThick = 0.5f * thickFrac * std::min(CELL_W, CELL_H);

                SubCellOBB obb;
                obb.center = C + glm::vec2(0.0f, PxToWorld(yNudgePx)); // <<< vertical nudge here
                obb.halfExtents = { halfAlong, halfThick };
                obb.tangent = T;
                obb.TileSlot = slot;
                m_blockedSubCells.push_back(obb);

            };

        auto emitCenteredDiscApprox = [&](const glm::ivec2& cell, float radiusFrac /*~0.18f*/, uint32_t slot)
            {
                const glm::vec2 S = IsoTileUtils::IsoToWorldGround(cell);
                const glm::vec2 E = S + glm::vec2(+CELL_W * 0.5f, -CELL_H * 0.5f);
                const glm::vec2 W = S + glm::vec2(-CELL_W * 0.5f, -CELL_H * 0.5f);
                const glm::vec2 N = S + glm::vec2(0.0f, -CELL_H);
                const glm::vec2 C = (E + W + N + S) * 0.25f;

                const float R = radiusFrac * 0.5f * std::min(CELL_W, CELL_H);

                // Two orthogonal strips crossing at C
                auto pushStrip = [&](const glm::vec2& T, uint32_t slot)
                    {
                        SubCellOBB obb;
                        obb.center = C;
                        obb.tangent = glm::normalize(T);
                        obb.halfExtents = { R, 0.5f * R };
                        obb.TileSlot = slot;
                        m_blockedSubCells.push_back(obb);


                    };

                pushStrip(E - W, slot);     // “horizontal”
                pushStrip(N - S,slot);     // “vertical”

            };

        // Per-side anchor correction used for walls (leave as-is)
        auto sideIsoOffset = [](FootSide s) -> glm::ivec2 {
            switch (s) {
            case FootSide::North: return { +1, +1 };
            case FootSide::South: return { +1, +1 };
            case FootSide::East:  return {  0, +1 };
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
                    emitEdgeSubcellsOnSide(cell, side, t.Slot);
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
                        emitCenteredDiscApprox(cell, 0.18f, t.Slot);
                    }
                    else
                    {
                        emitCenteredStrip(cell, kDefaultWidthFrac, kDefaultThickFrac, yNudgePx, t.Slot);
                    }
                }
                
                
            }
        }

        RebuildSubcellBuckets();
    }

    //return true of cell was destroyed
    bool GridMap::DamageSubCell(uint64_t key, float damage)
    {
        if (key == 0)
            return false;

        float& hp = m_subCellHealth[key];

        if (hp <= 0.0f)
            hp = 100.0f;

        hp -= damage;

        for (SubCellOBB& c : m_blockedSubCells)
        {
            if (c.CollisionKey == key)
            {
                c.Health = hp;
                break;
            }
        }

        if (hp <= 0.0f)
        {
            m_destroyedSubCells.insert(key);
            m_subCellHealth.erase(key);
            return true;
        }
        return false;
    }

    void GridMap::RemoveDeadSubCells()
    {
        m_blockedSubCells.erase(
            std::remove_if(
                m_blockedSubCells.begin(),
                m_blockedSubCells.end(),
                [&](const SubCellOBB& c)
                {
                    return c.Health <= 0.0f ||
                        m_destroyedSubCells.find(c.CollisionKey) != m_destroyedSubCells.end();
                }),
            m_blockedSubCells.end()
        );

        RebuildSubcellBuckets();
    }

    void GridMap::EmitEdgeSubcellsOnSide(const glm::ivec2& cell, FootSide side, uint32_t slot, uint64_t uid,
        float cellW, float cellH, int subs,float shrinkAlong, float halfThickW)
    {
        const glm::vec2 S = IsoTileUtils::IsoToWorldGround(cell);

        const glm::vec2 E = S + glm::vec2(+cellW * 0.5f, -cellH * 0.5f);
        const glm::vec2 W = S + glm::vec2(-cellW * 0.5f, -cellH * 0.5f);
        const glm::vec2 N = S + glm::vec2(0.0f, -cellH);
        const glm::vec2 C = E + W + N + S;

        glm::vec2 A{}, B{};

        switch (side)
        {
        case FootSide::North: A = N; B = E; break;
        case FootSide::East:  A = E; B = S; break;
        case FootSide::South: A = S; B = W; break;
        case FootSide::West:  A = W; B = N; break;
        }

        const glm::vec2 e = B - A;
        const float L = glm::length(e);
        if (L <= 1e-6f)
            return;

        const glm::vec2 T = e / L;
        glm::vec2 Nin = glm::vec2(-T.y, T.x);

        if (glm::dot(Nin, C - 0.5f * (A + B)) < 0.0f)
            Nin = -Nin;

        const float segLen = L / float(subs);
        const float halfAlong = 0.5f * segLen * shrinkAlong;

        for (int s = 0; s < subs; ++s)
        {
            const float t0 = float(s) * segLen;
            const float t1 = float(s + 1) * segLen;
            const float tm = 0.5f * (t0 + t1);
            const glm::vec2 P = A + T * tm;

            SubCellOBB obb{};
            obb.center = P + Nin * halfThickW;
            obb.halfExtents = { halfAlong, halfThickW };
            obb.tangent = T;
            obb.TileSlot = slot;

            obb.CollisionKey = GridUtils::MakeSubCellKey(
                uid,
                static_cast<uint32_t>(side),
                static_cast<uint32_t>(s)
            );

            if (m_destroyedSubCells.find(obb.CollisionKey) != m_destroyedSubCells.end())
                continue;

            auto it = m_subCellHealth.find(obb.CollisionKey);
            obb.Health = (it != m_subCellHealth.end()) ? it->second : 100.0f;

          
            m_blockedSubCells.push_back(obb);
        }
    }

    void GridMap::EmitCenteredStrip(const glm::ivec2& cell, float widthFrac, float thickFrac, float yNudgePx,
        uint32_t slot, uint64_t uid, float cellW, float cellH)
    {
        const glm::vec2 S = IsoTileUtils::IsoToWorldGround(cell);
        const glm::vec2 E = S + glm::vec2(+cellW * 0.5f, -cellH * 0.5f);
        const glm::vec2 W = S + glm::vec2(-cellW * 0.5f, -cellH * 0.5f);
        const glm::vec2 N = S + glm::vec2(0.0f, -cellH);
        const glm::vec2 C = (E + W + N + S) * 0.25f;

        const glm::vec2 T = glm::normalize(E - W);
        const float halfAlong = 0.5f * widthFrac * 0.5f * cellW;
        const float halfThick = 0.5f * thickFrac * std::min(cellW, cellH);

        SubCellOBB obb{};
        obb.center = C + glm::vec2(0.0f, PxToWorld(yNudgePx));
        obb.halfExtents = { halfAlong, halfThick };
        obb.tangent = T;
        obb.TileSlot = slot;

        obb.CollisionKey = GridUtils::MakeSubCellKey(
            uid,
            1000u, // centered strip type
            0u
        );

        if (m_destroyedSubCells.find(obb.CollisionKey) != m_destroyedSubCells.end())
            return;

        auto it = m_subCellHealth.find(obb.CollisionKey);
        obb.Health = (it != m_subCellHealth.end()) ? it->second : 100.0f;

       
        m_blockedSubCells.push_back(obb);
    }

    void GridMap::EmitCenteredDiscApprox(const glm::ivec2& cell, float radiusFrac, uint32_t slot,
        uint64_t uid, float cellW, float cellH)
    {
        const glm::vec2 S = IsoTileUtils::IsoToWorldGround(cell);
        const glm::vec2 E = S + glm::vec2(+cellW * 0.5f, -cellH * 0.5f);
        const glm::vec2 W = S + glm::vec2(-cellW * 0.5f, -cellH * 0.5f);
        const glm::vec2 N = S + glm::vec2(0.0f, -cellH);
        const glm::vec2 C = (E + W + N + S) * 0.25f;

        const float R = radiusFrac * 0.5f * std::min(cellW, cellH);

        auto PushStrip = [&](const glm::vec2& tangent, uint32_t subIndex)
            {
                SubCellOBB obb{};
                obb.center = C;
                obb.tangent = glm::normalize(tangent);
                obb.halfExtents = { R, 0.5f * R };
                obb.TileSlot = slot;

                obb.CollisionKey = GridUtils::MakeSubCellKey(
                    uid,
                    2000u, // disc approximation type
                    subIndex
                );

                if (m_destroyedSubCells.find(obb.CollisionKey) != m_destroyedSubCells.end())
                    return;

                auto it = m_subCellHealth.find(obb.CollisionKey);
                obb.Health = (it != m_subCellHealth.end()) ? it->second : 100.0f;
             
                m_blockedSubCells.push_back(obb);
            };

        PushStrip(E - W, 0u);
        PushStrip(N - S, 1u);
    }

    void GridMap::BuildFromTilesNearPlayer(Scene* scene, const glm::vec2& playerWorldPos, float radiusWorld)
    {
        EE_PROFILE_FUNCTION();

        if (!scene)
            return;

        m_blockedSubCells.clear();
        m_cellToSubcells.clear();

        // --- measure iso diamond in world units ---
        const glm::vec2 g00 = IsoTileUtils::IsoToWorldGround({ 0,0 });
        const glm::vec2 gE = IsoTileUtils::IsoToWorldGround({ 1,-1 });
        const glm::vec2 gS = IsoTileUtils::IsoToWorldGround({ 1, 1 });

        const float CELL_W = std::abs(gE.x - g00.x);
        const float CELL_H = std::abs(gS.y - g00.y);
        if (CELL_W <= 0.0f || CELL_H <= 0.0f)
            return;

        constexpr int   SUBS = SUBDIVS;
        constexpr float SHRINK_ALONG = 0.96f;
        const float HALF_THICK_W = 0.5f * (0.22f * std::min(CELL_W, CELL_H));


        auto parseSide = [](const std::string& name) -> FootSide
            {
                auto pos = name.find_last_of('_');
                if (pos != std::string::npos && pos + 1 < name.size())
                {
                    char c = (char)std::toupper(name[pos + 1]);
                    if (c == 'N') return FootSide::North;
                    if (c == 'E') return FootSide::East;
                    if (c == 'S') return FootSide::South;
                    if (c == 'W') return FootSide::West;
                }
                return FootSide::South;
            };


        auto sideIsoOffset = [](FootSide s) -> glm::ivec2
            {
                switch (s)
                {
                case FootSide::North: return { +1, +2 };
                case FootSide::South: return { +1, +0 };
                case FootSide::West:  return { +1, +1 };
                case FootSide::East:  return { +1, +1 };
                }
                return { 0, 0 };
            };

        const float radiusSq = radiusWorld * radiusWorld;

        auto view = scene->GetRegistry().view<TileComponent, TransformComponent>();
        for (auto e : view)
        {
            const auto& tc = view.get<TileComponent>(e);
            const auto& tr = view.get<TransformComponent>(e);

            for (const auto& t : tc.tiles)
            {
                if (t.Category == eTileCategory::Terrain)
                    continue;

                if (t.IsSpawned)
                {
                    // this tile was spawned from destuction system so dont make new grid for it
                    continue;
                }

                const glm::vec2 ground = glm::vec2(tr.Translation) + glm::vec2(t.position);

                if (glm::length2(ground - playerWorldPos) > radiusSq)
                    continue;

                glm::ivec2 cell = IsoTileUtils::WorldToIsoCell(ground);

                if (t.Category == eTileCategory::Windows)
                {
                    // same as building but needs tweak in future(jump trough window)?
                    FootSide side = parseSide(t.name);
                    cell += sideIsoOffset(side);
                    EmitEdgeSubcellsOnSide(cell, side, t.Slot, t.UID, CELL_W, CELL_H, SUBS, SHRINK_ALONG, HALF_THICK_W);
                }
                else if (t.Category == eTileCategory::Buildings)
                {
                    FootSide side = parseSide(t.name);
                    cell += sideIsoOffset(side);
                    EmitEdgeSubcellsOnSide(cell, side, t.Slot, t.UID, CELL_W, CELL_H, SUBS, SHRINK_ALONG, HALF_THICK_W);
                }
                else if (t.Category == eTileCategory::dynamicObjects)
                {
                    constexpr float kDefaultWidthFrac = 0.35f;
                    constexpr float kDefaultThickFrac = 0.12f;
                    constexpr float yNudgePx = 90.f;
                    constexpr bool  kUseDiscForPosts = false;

                    if (kUseDiscForPosts)
                        EmitCenteredDiscApprox(cell, 0.18f, t.Slot, t.UID, CELL_W, CELL_H);
                    else
                        EmitCenteredStrip(cell, kDefaultWidthFrac, kDefaultThickFrac, yNudgePx, t.Slot, t.UID, CELL_W, CELL_H);
                }
            }
        }

        RebuildSubcellBuckets();
    }


    void GridMap::UpdateCollisionAroundPlayer(Scene* scene, const glm::vec2& playerWorldPos)
    {

        if (glm::length2(playerWorldPos - m_LastGridBuildPos) < 4.0f)
            return;

        m_LastGridBuildPos = playerWorldPos;

        BuildFromTilesNearPlayer(scene, playerWorldPos, 25.0f);
    }

    void GridMap::MarkBlockedSubtilesFromTexture( const glm::vec2& worldPosition,
        const std::vector<uint8_t>& textureData, uint32_t textureWidth, uint32_t textureHeight)
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







    bool GridMap::HasLineOfSight(glm::vec2 fromWorld, glm::vec2 toWorld, bool debugDraw)
    {
       // EE_PROFILE_FUNCTION();

        constexpr float subtileSize = float(TILE_SIZE) / float(GRID_SUBDIVISIONS);

        glm::vec2 seg = toWorld - fromWorld;
        float segLen = glm::length(seg);
        if (segLen < 1e-4f)
            return true; // same point

        glm::vec2 dir = seg / segLen;

        // Step roughly one subtile at a time
        const float stepLen = subtileSize * 0.8f; // slightly denser than subtiles
        const int   maxSteps = (int)glm::ceil(segLen / stepLen);

        bool isFirstSample = true;

        // Debug: draw the final LOS line
        if (debugDraw)
        {
            DrawDebugLine(fromWorld, toWorld, glm::vec4(0, 1, 0, 1)); // default green, overridden on block
        }

        for (int i = 0; i <= maxSteps; ++i)
        {
            float t = (maxSteps > 0) ? (float)i / (float)maxSteps : 0.0f;
            glm::vec2 P = glm::mix(fromWorld, toWorld, t);

            // Skip the very first point so we don't consider the shooter "blocking" itself
            if (!isFirstSample)
            {
                bool blockedHere = false;

                // Test against all collision subcells
                for (const auto& obb : m_blockedSubCells)
                {
                    constexpr float kLOSObstaclePadding = 0.2f;
                    if (GridUtils::PointInSubCellOBB_Padded(P, obb, kLOSObstaclePadding))
                    {
                        blockedHere = true;

                        if (debugDraw)
                        {
                            // Draw a small red square at the blocking point
                            glm::mat4 model =
                                glm::translate(glm::mat4(1.0f), glm::vec3(P, 0.0f)) *
                                glm::scale(glm::mat4(1.0f), glm::vec3(subtileSize));

                            Engine::VulkanRenderer2D::DrawLineRect(
                                model, glm::vec4(1, 0, 0, 0.5f), -1.0f);

                            DrawDebugLine(fromWorld, toWorld, glm::vec4(1, 0, 0, 1)); // red LOS
                        }

                        return false; // line of sight blocked
                    }
                }

                if (debugDraw && !blockedHere)
                {
                    // Visualize sampled LOS points
                    glm::mat4 model =
                        glm::translate(glm::mat4(1.0f), glm::vec3(P, 0.0f)) *
                        glm::scale(glm::mat4(1.0f), glm::vec3(subtileSize * 0.5f));

                    Engine::VulkanRenderer2D::DrawLineRect(
                        model, glm::vec4(1, 1, 0, 0.2f), -1.0f);
                }
            }

            isFirstSample = false;
        }

        if (debugDraw)
        {
            // Mark the endpoint as visible
            glm::mat4 model =
                glm::translate(glm::mat4(1.0f), glm::vec3(toWorld, 0.0f)) *
                glm::scale(glm::mat4(1.0f), glm::vec3(subtileSize));

            Engine::VulkanRenderer2D::DrawLineRect(
                model, glm::vec4(0, 1, 1, 0.4f), -1.0f);
            // LOS line already drawn (green) at the start
        }

        return true; // no collision along the ray
    }

   
    void GridMap::UpdateTiles()
    {
        EE_PROFILE_FUNCTION();

        const auto& tiles = Engine::TileBlockedMaskCPU::DirtyTileRuntime;
        if (tiles.empty() || m_blockedSubCells.empty())
            return;

        // Tunables
        static constexpr float MIN_DEAD_FRAC = 0.50f; // >=50% of inside pixels dead -> delete
        static constexpr int   MIN_DEAD_ABS = 3;     // at least a few pixels must be dead

        // Tile footprint in world units (must match destructible)
        const glm::vec2 tileSizeW((float)TILE_SIZE, (float)TILE_SIZE);

        // Texture resolution
        const int TPW = TILE_PIXEL_WIDTH;
        const int TPH = TILE_PIXEL_HEIGHT;

        // Bitfield helpers (aliveWords: 1=alive, 0=dead)
        auto maskOK = [&](const std::vector<uint32_t>& words)->bool {
            const int need = ((TPW * TPH) + 31) / 32;
            return (int)words.size() >= need;
            };
        auto isDead = [&](const std::vector<uint32_t>& words, int x, int y)->bool {
            if ((unsigned)x >= (unsigned)TPW || (unsigned)y >= (unsigned)TPH) return false;
            const int idx = y * TPW + x;
            const int word = idx >> 5;
            const int bit = idx & 31;
            return (((words[word] >> bit) & 1u) == 0u);
            };

        // Build kill flags
        std::vector<uint8_t> kill(m_blockedSubCells.size(), 0);

        for (const auto& tr : tiles)
        {
            if (!maskOK(tr.aliveWords)) continue;

            // World -> this tile's pixel space
            const glm::vec2 tileMinW = tr.topLeft;
            const float sx = float(TPW) / tileSizeW.x; // 1 / pxW
            const float sy = float(TPH) / tileSizeW.y; // 1 / pxH

            auto WorldToPx = [&](const glm::vec2& Pw)->glm::vec2 {
                return { (Pw.x - tileMinW.x) * sx, (Pw.y - tileMinW.y) * sy };
                };

            // Iterate only subcells that belong to this slot
            for (size_t si = 0; si < m_blockedSubCells.size(); ++si)
            {
                if (kill[si]) continue;
                const SubCellOBB& obb = m_blockedSubCells[si];
                if (obb.TileSlot != tr.slot) continue;

                // Ownership guard: subcell center must lie inside THIS tile AABB
                const glm::vec2 cW = obb.center;
                if (cW.x < tileMinW.x || cW.y < tileMinW.y ||
                    cW.x >= tileMinW.x + tileSizeW.x || cW.y >= tileMinW.y + tileSizeW.y)
                    continue;

                // Compute OBB frame in pixel space (handles non-uniform scale)
                glm::vec2 T = obb.tangent;
                float tlen = glm::length(T);
                if (tlen < 1e-8f) T = { 1,0 }; else T /= tlen;
                glm::vec2 N = { -T.y, T.x };

                glm::vec2 Cpx = WorldToPx(obb.center);
                glm::vec2 Tpx = { T.x * sx, T.y * sy };
                glm::vec2 Npx = { N.x * sx, N.y * sy };

                float lenTpx = glm::length(Tpx);
                float lenNpx = glm::length(Npx);
                if (lenTpx < 1e-12f || lenNpx < 1e-12f) continue;

                glm::vec2 Tpx_hat = Tpx / lenTpx;
                glm::vec2 Npx_hat = Npx / lenNpx;
                float hx_px = obb.halfExtents.x * lenTpx;
                float hy_px = obb.halfExtents.y * lenNpx;

                // OBB AABB in pixel space -> integer window
                auto corner = [&](float su, float sv)->glm::vec2 {
                    return Cpx + Tpx_hat * (su * hx_px) + Npx_hat * (sv * hy_px);
                    };
                glm::vec2 c0 = corner(-1, -1), c1 = corner(+1, -1);
                glm::vec2 c2 = corner(+1, +1), c3 = corner(-1, +1);
                float minx = std::min(std::min(c0.x, c1.x), std::min(c2.x, c3.x));
                float maxx = std::max(std::max(c0.x, c1.x), std::max(c2.x, c3.x));
                float miny = std::min(std::min(c0.y, c1.y), std::min(c2.y, c3.y));
                float maxy = std::max(std::max(c0.y, c1.y), std::max(c2.y, c3.y));

                int x0 = std::clamp((int)std::floor(minx), 0, TPW);
                int x1 = std::clamp((int)std::ceil(maxx), 0, TPW);
                int y0 = std::clamp((int)std::floor(miny), 0, TPH);
                int y1 = std::clamp((int)std::ceil(maxy), 0, TPH);
                if (x1 <= x0 || y1 <= y0) continue;

                // Count dead pixel centers inside OBB
                const int totalWin = (x1 - x0) * (y1 - y0);
                const int needDead = std::max(MIN_DEAD_ABS, int(std::ceil(MIN_DEAD_FRAC * float(totalWin))));
                int deadCount = 0;

                for (int py = y0; py < y1 && deadCount < needDead; ++py)
                {
                    const float cy = float(py) + 0.5f;
                    for (int px = x0; px < x1 && deadCount < needDead; ++px)
                    {
                        const float cx = float(px) + 0.5f;
                        glm::vec2 d = { cx - Cpx.x, cy - Cpx.y };
                        const float u = glm::dot(d, Tpx_hat);
                        const float v = glm::dot(d, Npx_hat);
                        if (std::abs(u) <= hx_px && std::abs(v) <= hy_px) {
                            if (isDead(tr.aliveWords, px, py)) ++deadCount;
                        }
                    }
                }

                if (deadCount >= needDead)
                    kill[si] = 1;
            }
        }

        // Compact deleted subcells in-place
        size_t w = 0;
        for (size_t i = 0; i < m_blockedSubCells.size(); ++i)
            if (!kill[i]) m_blockedSubCells[w++] = m_blockedSubCells[i];
        m_blockedSubCells.resize(w);
    }


    bool GridMap::IsCellBlocked(const glm::ivec2& cell) const
    {
        // pick a representative point for the cell (its “ground” center)
        glm::vec2 point = IsoTileUtils::IsoToWorldGround(cell);

        for (const auto& obb : m_blockedSubCells)
        {
            constexpr float kLOSObstaclePadding = 0.1f;

            if (GridUtils::PointInSubCellOBB_Padded(point, obb, kLOSObstaclePadding))
            {
                return true;
            }
        }
        return false;
    }

    bool GridMap::IsPointBlockedWithNormal(const glm::vec2& P, glm::vec2& outNormal) const
    {
        constexpr float kPadding = 0.1f;

        for (const auto& obb : m_blockedSubCells)
        {
            // Same test as before
            if (GridUtils::PointInSubCellOBB_Padded(P, obb, kPadding))
            {
                const glm::vec2 T = obb.tangent;                // assumed normalized or close
                const glm::vec2 N = glm::vec2(-T.y, T.x);       // normal

                outNormal = glm::normalize(N);
                return true;
            }
        }
        return false;
    }



    std::vector<glm::vec2> GridMap::FindPathWorld(const glm::vec2& startWorld, const glm::vec2& goalWorld) const
    {
        EE_PROFILE_FUNCTION();
        glm::ivec2 startCell = IsoTileUtils::WorldToIsoCell(startWorld);
        glm::ivec2 goalCell = IsoTileUtils::WorldToIsoCell(goalWorld);

        if (startCell == goalCell)
            return { goalWorld }; // trivial

        struct Node {
            glm::ivec2 cell;
            float fScore;
        };

        struct NodeCmp {
            bool operator()(const Node& a, const Node& b) const {
                return a.fScore > b.fScore; // min-heap
            }
        };

        struct IVec2Hasher {
            size_t operator()(const glm::ivec2& v) const noexcept {
                uint64_t x = (uint32_t)v.x;
                uint64_t y = (uint32_t)v.y;
                return std::hash<uint64_t>{}((x << 32) ^ y);
            }
        };

        auto heuristic = [](const glm::ivec2& a, const glm::ivec2& b) -> float {
            glm::ivec2 d = b - a;
            // diagonal distance
            int dx = std::abs(d.x);
            int dy = std::abs(d.y);
            int diag = std::min(dx, dy);
            int straight = std::max(dx, dy) - diag;
            return diag * 1.4142f + straight * 1.0f;
            };

        std::priority_queue<Node, std::vector<Node>, NodeCmp> open;

        std::unordered_map<glm::ivec2, glm::ivec2, IVec2Hasher> cameFrom;
        std::unordered_map<glm::ivec2, float, IVec2Hasher> gScore;

        auto key = [&](const glm::ivec2& c) -> glm::ivec2 { return c; };

        gScore[key(startCell)] = 0.0f;

        open.push({ startCell, heuristic(startCell, goalCell) });

        const glm::ivec2 neighbors[8] = {
            {+1, 0}, {-1, 0}, {0,+1}, {0,-1},
            {+1,+1}, {+1,-1}, {-1,+1}, {-1,-1}
        };





        const int kMargin = 64; // tune
        glm::ivec2 mn(std::min(startCell.x, goalCell.x) - kMargin,
            std::min(startCell.y, goalCell.y) - kMargin);
        glm::ivec2 mx(std::max(startCell.x, goalCell.x) + kMargin,
            std::max(startCell.y, goalCell.y) + kMargin);

        auto inWindow = [&](const glm::ivec2& c) -> bool
            {
                return (c.x >= mn.x && c.y >= mn.y && c.x <= mx.x && c.y <= mx.y);
            };

        std::unordered_map<glm::ivec2, uint8_t, IVec2Hasher> blockedCache;
        blockedCache.reserve(4096);

        auto isBlocked = [&](const glm::ivec2& c) -> bool
            {
                if (!inWindow(c)) return true;

                auto it = blockedCache.find(c);
                if (it != blockedCache.end()) return it->second != 0;

                bool b = IsCellBlocked(c);
                blockedCache.emplace(c, b ? 1 : 0);
                return b;
            };
        const float costStraight = 1.0f;
        const float costDiag = 1.4142f;

        std::unordered_set<glm::ivec2, IVec2Hasher> closed;

        bool found = false;


        const int MAX_EXPANSIONS = 2000;
        int expansions = 0;
        while (!open.empty())
        {

            if (++expansions > MAX_EXPANSIONS)
            {
                EE_CORE_WARN("[Path] Abort expansions>{} start=({}, {}) goal=({}, {})",
                    MAX_EXPANSIONS, startCell.x, startCell.y, goalCell.x, goalCell.y);
                break;
            }

            glm::ivec2 current = open.top().cell;
            open.pop();

            if (current == goalCell) {
                found = true;
                break;
            }

            if (closed.find(current) != closed.end())
                continue;
            closed.insert(current);

            for (int i = 0; i < 8; ++i)
            {
                glm::ivec2 n = current + neighbors[i];

                if (isBlocked(n))
                    continue;

                // Optional: prevent squeezing through corners
                if (i >= 4) {
                    glm::ivec2 n1(current.x, n.y);
                    glm::ivec2 n2(n.x, current.y);
                    if (isBlocked(n1) && isBlocked(n2))
                        continue;
                }

                float stepCost = (i < 4) ? costStraight : costDiag;

                float tentativeG = gScore[key(current)] + stepCost;

                auto itG = gScore.find(key(n));
                if (itG != gScore.end() && tentativeG >= itG->second)
                    continue;

                cameFrom[key(n)] = current;
                gScore[key(n)] = tentativeG;

                float f = tentativeG + heuristic(n, goalCell);
                open.push({ n, f });
            }
        }

        std::vector<glm::vec2> result;

        if (!found) {
            // fallback: direct goal if found no path
            result.push_back(goalWorld);
            return result;
        }

        // Reconstruct path in reverse
        std::vector<glm::ivec2> cells;
        glm::ivec2 cur = goalCell;
        cells.push_back(cur);
        while (cur != startCell)
        {
            auto it = cameFrom.find(key(cur));
            if (it == cameFrom.end())
                break;
            cur = it->second;
            cells.push_back(cur);
        }

        std::reverse(cells.begin(), cells.end());

        // Convert cell centers back to world-space points
        result.reserve(cells.size());
        for (const auto& c : cells)
        {
            glm::vec2 w = IsoTileUtils::IsoToWorldGround(c);
            result.push_back(w);
        }


        
        return result;
    }

    void GridMap::DebugDrawPath(const std::vector<glm::vec3>& path) const
    {
        if (path.size() < 2)
            return;

        const float nodeSize = 0.1f;

        for (size_t i = 0; i < path.size(); ++i)
        {
            glm::vec3 p = path[i];

            glm::mat4 model =
                glm::translate(glm::mat4(1.0f), glm::vec3(p.x, p.y, 0.0f)) *
                glm::scale(glm::mat4(1.0f), glm::vec3(nodeSize));

            glm::vec4 color;
            if (i == 0)                      color = glm::vec4(0, 1, 0, 0.6f);
            else if (i + 1 == path.size())   color = glm::vec4(1, 0, 0, 0.6f);
            else                             color = glm::vec4(1, 1, 0, 0.4f);

            Engine::VulkanRenderer2D::DrawLineRect(model, color, -1.0f);

            if (i + 1 < path.size())
            {
                glm::vec2 q = path[i + 1];
                DrawDebugLine(glm::vec2(p.x, p.y),
                    glm::vec2(q.x, q.y),
                    glm::vec4(0, 0.7f, 1.0f, 1.0f));
            }
        }
    }


	void GridMap::DrawDebugLine(glm::vec2 from, glm::vec2 to, const glm::vec4& color) const
	{
       // EE_PROFILE_FUNCTION();
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


    void GridMap::RebuildSubcellBuckets()
    {
        //EE_INFO("RebuildSubcellBuckets");
        m_cellToSubcells.clear();
        m_cellToSubcells.reserve(m_blockedSubCells.size() * 2);

        for (int i = 0; i < (int)m_blockedSubCells.size(); ++i)
        {
            const SubCellOBB& obb = m_blockedSubCells[i];

            // Bucket by iso cell of the OBB center
            glm::ivec2 cell = IsoTileUtils::WorldToIsoCell(obb.center);

            m_cellToSubcells[PackXY(cell.x, cell.y)].push_back(i);
        }
    }

    bool GridMap::IsPointBlocked_Bucketed(const glm::vec2& P, float padding) const
    {
        glm::ivec2 c = IsoTileUtils::WorldToIsoCell(P);

        // Check this cell + neighbors (important because your OBBs can sit on edges)
        for (int oy = -1; oy <= 1; ++oy)
            for (int ox = -1; ox <= 1; ++ox)
            {
                uint64_t key = PackXY(c.x + ox, c.y + oy);
                auto it = m_cellToSubcells.find(key);
                if (it == m_cellToSubcells.end())
                    continue;

                const std::vector<int>& indices = it->second;
                for (int idx : indices)
                {
                    // defensive (in case of stale data)
                    if ((unsigned)idx >= (unsigned)m_blockedSubCells.size())
                        continue;

                    const SubCellOBB& obb = m_blockedSubCells[idx];
                    if (GridUtils::PointInSubCellOBB_Padded(P, obb, padding))
                        return true;
                }
            }

        return false;
    }
}
