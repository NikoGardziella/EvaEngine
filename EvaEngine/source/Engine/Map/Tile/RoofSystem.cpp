// RoofSystem.cpp
#include "pch.h"
#include "RoofSystem.h"

#include <algorithm>
#include <queue>
#include <unordered_set>
#include <Engine/Core/Assert.h>
#include "Utils/RoofUtils.h"


namespace Engine {

   
    void RoofSystem::Init(Scene* scene, SceneRoofTileAccess* access, const RoofSystemConfig& cfg)
    {
        m_scene = scene;
        m_access = access;
        m_cfg = cfg;

        RoofSystemState& st = StateFor(this);
        st.dirty.clear();
        st.dirtyMerged.clear();
        st.pending.clear();
    }

    void RoofSystem::Update(float dt)
    {
        EE_PROFILE_FUNCTION();

        if (!m_access) return;

        RoofSystemState& st = StateFor(this);

        // Merge dirty regions for this frame
        RoofUtils::MergeDirtyAABBs(st);

        // Evaluate stability in dirty areas and schedule collapses
        EvaluateDirtyRegionsAndScheduleCollapses(st, m_access, m_cfg);

        // Apply collapses after delay
        ProcessPendingCollapses(st, m_access, m_cfg, dt);
    }

    void RoofSystem::NotifySupportLostAt(const glm::ivec2& supportCell)
    {
        RoofSystemState& st = StateFor(this);

        glm::ivec2 roofCell = supportCell + glm::ivec2(0, m_cfg.roofSupportYOffsetCells);


        RoofUtils::MarkDirtyRadius(st, roofCell, m_cfg.dirtyRadiusOnSupportChange);
    }

    void RoofSystem::NotifySupportLostAtWorld(const glm::vec2& supportWorld)
    {

        const int ix = RoofUtils::Quantize(supportWorld.x, ROOF_STEP_X);
        const int iy = RoofUtils::Quantize(supportWorld.y, ROOF_STEP_Y);

        NotifySupportLostAt({ ix, iy });
    }

    void RoofSystem::NotifySupportAddedAt(const glm::ivec2& supportPos)
    {
        RoofSystemState& st = StateFor(this);
        RoofUtils::MarkDirtyRadius(st, supportPos, m_cfg.dirtyRadiusOnSupportChange);
    }



    RoofSystemState& RoofSystem::StateFor(Engine::RoofSystem* self)
    {
        static std::unordered_map<RoofSystem*, RoofSystemState> s_states;
        return s_states[self];
    }


   

    void RoofSystem::FloodFillRoofComponent(const SceneRoofTileAccess* access, const RoofSystemConfig& cfg,
        const glm::ivec2& seed, const glm::ivec2& regionMin, const glm::ivec2& regionMax,   std::vector<uint8_t>& visited, int regionW,
        int regionH, std::vector<glm::ivec2>& outTiles)
    {
        EE_PROFILE_FUNCTION();

        outTiles.clear();
        std::vector<glm::ivec2> stack;
        stack.reserve(256);
        stack.push_back(seed);

        auto toLocal = [&](const glm::ivec2& p, int& lx, int& ly)
            {
                lx = p.x - regionMin.x;
                ly = p.y - regionMin.y;
            };

        int sx, sy;
        toLocal(seed, sx, sy);
        if (sx < 0 || sy < 0 || sx >= regionW || sy >= regionH) return;

        visited[RoofUtils::LocalIndex(sx, sy, regionW)] = 1;

        static const int DX4[4] = { +1,-1, 0, 0 };
        static const int DY4[4] = { 0, 0,+1,-1 };

        static const int DX8[8] = { +1,-1, 0, 0, +1,+1,-1,-1 };
        static const int DY8[8] = { 0, 0,+1,-1, +1,-1,+1,-1 };

        const bool use8 = cfg.use8Connected;
        const int* DX = use8 ? DX8 : DX4;
        const int* DY = use8 ? DY8 : DY4;
        const int K = use8 ? 8 : 4;

        while (!stack.empty())
        {
            glm::ivec2 p = stack.back();
            stack.pop_back();

            outTiles.push_back(p);

            for (int k = 0; k < K; ++k)
            {
                glm::ivec2 n(p.x + DX[k], p.y + DY[k]);

                if (n.x < regionMin.x || n.x > regionMax.x || n.y < regionMin.y || n.y > regionMax.y)
                    continue;


                int lx, ly;
                toLocal(n, lx, ly);
                if (lx < 0 || ly < 0 || lx >= regionW || ly >= regionH)
                    continue;

                const size_t li = RoofUtils::LocalIndex(lx, ly, regionW);


                if (visited[li]) continue;
                visited[li] = 1;

                if (!access->HasRoof(n))
                    continue;

                stack.push_back(n);
            }
        }
    }


    bool RoofSystem::PendingAlreadyContainsAny(const RoofSystemState& st, const std::vector<glm::ivec2>& tiles)
    {
        for (const PendingCollapse& pc : st.pending)
        {
            for (const glm::ivec2& t : tiles)
            {
                if (std::find(pc.tiles.begin(), pc.tiles.end(), t) != pc.tiles.end())
                    return true;
            }
        }
        return false;
    }

    void RoofSystem::ScheduleCollapse(RoofSystemState& st, const RoofSystemConfig& cfg, const std::vector<glm::ivec2>& tiles)
    {
        if (tiles.empty())
        {
            return;
        }
        if (PendingAlreadyContainsAny(st, tiles))
        {
            return;
        }

        PendingCollapse pc;
        pc.tiles = tiles;
        pc.timeLeft = cfg.collapseDelaySeconds;
        st.pending.push_back(std::move(pc));


    }

    bool RoofSystem::IsRoofCellSupported(const SceneRoofTileAccess* access, const RoofSystemConfig& cfg,
        const glm::ivec2& roofCell)
    {
        // Map roof cell to the support plane
        const glm::ivec2 baseSupportCell = roofCell - glm::ivec2(0, cfg.roofSupportYOffsetCells);

        int found = 0;

        const int r = cfg.supportSearchRadiusCells;
        for (int dy = -r; dy <= r; ++dy)
        {
            for (int dx = -r; dx <= r; ++dx)
            {
                const glm::ivec2 s = baseSupportCell + glm::ivec2(dx, dy);

                if (!access->HasSupport(s)) continue;

                if (++found >= cfg.minSupportsPerRoofTile)
                    return true;
            }
        }

        return false;
    }

    void RoofSystem::EvaluateDirtyRegionsAndScheduleCollapses(RoofSystemState& st,  const SceneRoofTileAccess* access,
        const RoofSystemConfig& cfg)
    {
        EE_PROFILE_FUNCTION();

        if (st.dirtyMerged.empty()) return;

        std::vector<glm::ivec2> compTiles;
        compTiles.reserve(512);

        for (const DirtyAABB& reg : st.dirtyMerged)
        {
            const glm::ivec2 regionMin = reg.min;
            const glm::ivec2 regionMax = reg.max;

            const int regionW = regionMax.x - regionMin.x + 1;
            const int regionH = regionMax.y - regionMin.y + 1;
            if (regionW <= 0 || regionH <= 0) continue;

            std::vector<uint8_t> visited((size_t)regionW * (size_t)regionH, 0);

            for (int ry = 0; ry < regionH; ++ry)
            {
                for (int rx = 0; rx < regionW; ++rx)
                {
                    const size_t li = RoofUtils::LocalIndex(rx, ry, regionW);
                    if (visited[li]) continue;

                    glm::ivec2 p(regionMin.x + rx, regionMin.y + ry);

                    // Mark visited even if out of bounds, to avoid reprocessing
                    visited[li] = 1;



                    if (!access->HasRoof(p))
                    {
                        // EE_CORE_INFO("evaluating dirty region, does not have roof{}", p);

                        continue;
                    }

                    // p is a roof tile seed, do flood fill within region
                    FloodFillRoofComponent(access, cfg, p, regionMin, regionMax, visited, regionW, regionH, compTiles);
                    if (compTiles.empty()) continue;

                    std::vector<glm::ivec2> unsupported;
                    unsupported.reserve(compTiles.size());

                    for (const glm::ivec2& roofCell : compTiles)
                    {
                        if (!IsRoofCellSupported(access, cfg, roofCell))
                        {

                            unsupported.push_back(roofCell);
                        }
                    }

                    if (!unsupported.empty())
                    {
                        // simplest: collapse all unsupported tiles
                        ScheduleCollapse(st, cfg, unsupported);




                    }
                }
            }
        }

        st.dirtyMerged.clear();
    }

    void RoofSystem::ProcessPendingCollapses(RoofSystemState& st,  SceneRoofTileAccess* access,
        const RoofSystemConfig& cfg, float dt)
    {
        EE_PROFILE_FUNCTION();

        for (size_t i = 0; i < st.pending.size(); )
        {
            PendingCollapse& pc = st.pending[i];
            pc.timeLeft -= dt;

            if (pc.timeLeft > 0.0f)
            {
                ++i;
                continue;
            }
            for (const glm::ivec2& p : pc.tiles)
            {
                if (!access->HasRoof(p)) continue;

                access->RemoveRoof(p);
            }

            // Mark area a bit dirty after collapse, because components changed
            for (const glm::ivec2& p : pc.tiles)
                RoofUtils::MarkDirtyRadius(st, p, 1);

            st.pending.erase(st.pending.begin() + i);
        }

        (void)cfg;
    }


} 