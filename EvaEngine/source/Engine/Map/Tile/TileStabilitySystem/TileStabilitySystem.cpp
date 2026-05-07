#include "pch.h"
#include "TileStabilitySystem.h"

#include "Engine/Scene/Scene.h"
#include "Engine/Scene/Entity.h"

#include <algorithm>
#include <queue>
#include "Engine/Map/Utils/IsoTileUtils.h"
#include "Engine/Scene/Components/Physics/PhysicUtils.h"
#include <Engine/Math/HashUtils.h>
#include "Engine/AssetManager/AssetManager.h"

namespace Engine
{
    void TileStabilitySystem::InitTileStabilitySystem(Scene* scene)
    {
        m_scene = scene;
    }
    void TileStabilitySystem::Update(float dt)
    {
        EE_PROFILE_FUNCTION();

        if (!m_scene)
            return;

        MergeDirtyAABBs();
        EvaluateDirtyRegionsAndScheduleCollapses();
        ProcessPendingCollapses(dt);
    }

    void TileStabilitySystem::NotifySupportLostAtCell(const glm::ivec2& cell, int16_t floor)
    {
        const int r = m_cfg.DirtyRadiusCells;
        const int SameFloorDirtyRadiusCells = 4;
        MarkDirtyAABB(cell - glm::ivec2(SameFloorDirtyRadiusCells, SameFloorDirtyRadiusCells), cell + glm::ivec2(SameFloorDirtyRadiusCells, SameFloorDirtyRadiusCells), floor);
        MarkDirtyAABB(cell - glm::ivec2(r, r), cell + glm::ivec2(r, r), static_cast<int16_t>(floor + 1));
    }

    void TileStabilitySystem::MarkDirtyAABB(const glm::ivec2& min, const glm::ivec2& max, int16_t floor)
    {
        DirtyStabilityAABB reg;
        reg.Min = min;
        reg.Max = max;
        reg.Floor = floor;

        m_dirty.push_back(reg);
    }

    void TileStabilitySystem::MergeDirtyAABBs()
    {
        m_dirtyMerged.clear();

        for (const DirtyStabilityAABB& src : m_dirty)
        {
            bool merged = false;

            for (DirtyStabilityAABB& dst : m_dirtyMerged)
            {
                if (dst.Floor != src.Floor)
                    continue;

                const bool overlaps = src.Min.x <= dst.Max.x + 1 &&
                    src.Max.x >= dst.Min.x - 1 &&
                    src.Min.y <= dst.Max.y + 1 &&
                    src.Max.y >= dst.Min.y - 1;

                if (!overlaps)
                    continue;

                dst.Min.x = std::min(dst.Min.x, src.Min.x);
                dst.Min.y = std::min(dst.Min.y, src.Min.y);
                dst.Max.x = std::max(dst.Max.x, src.Max.x);
                dst.Max.y = std::max(dst.Max.y, src.Max.y);

                merged = true;
                break;
            }

            if (!merged)
                m_dirtyMerged.push_back(src);
        }

        m_dirty.clear();
    }

    void TileStabilitySystem::EvaluateDirtyRegionsAndScheduleCollapses()
    {
        if (!m_scene || m_dirtyMerged.empty())
            return;

        std::vector<glm::ivec2> componentCells;
        componentCells.reserve(512);

        for (const DirtyStabilityAABB& reg : m_dirtyMerged)
        {
            const glm::ivec2 regionMin = reg.Min;
            const glm::ivec2 regionMax = reg.Max;
            const int16_t floor = reg.Floor;

            const int regionW = regionMax.x - regionMin.x + 1;
            const int regionH = regionMax.y - regionMin.y + 1;

            if (regionW <= 0 || regionH <= 0)
                continue;

            std::vector<uint8_t> visited(static_cast<size_t>(regionW) * static_cast<size_t>(regionH), 0);

            for (int ry = 0; ry < regionH; ++ry)
            {
                for (int rx = 0; rx < regionW; ++rx)
                {
                    const size_t li = LocalIndex(rx, ry, regionW);

                    if (visited[li])
                        continue;

                    visited[li] = 1;

                    const glm::ivec2 p(regionMin.x + rx, regionMin.y + ry);

                    if (!HasStabilityTile(p, floor))
                        continue;

                    componentCells.clear();

                    FloodFillComponent(p, floor, regionMin, regionMax, visited, regionW, regionH, componentCells);

                    if (componentCells.empty())
                        continue;
                    int supportedCount = 0;

                    for (const glm::ivec2& cell : componentCells)
                    {
                        if (IsTileSupported(cell, floor))
                            ++supportedCount;
                    }

                    float supportRatio = componentCells.empty()
                        ? 1.0f
                        : float(supportedCount) / float(componentCells.size());

                    bool isRoofComponent = false;

                    for (const glm::ivec2& cell : componentCells)
                    {
                        if (IsRoofTile(cell, floor))
                        {
                            isRoofComponent = true;
                            break;
                        }
                    }

                    float collapseThreshold = isRoofComponent ? 0.15f : 0.75f;
                    
                    if (supportRatio <= collapseThreshold)
                    {
                        EE_CORE_INFO("supportRatio {}", supportRatio);
                        ScheduleCollapse(componentCells, floor);
                    }

                }
            }
        }

        m_dirtyMerged.clear();
    }

    void TileStabilitySystem::FloodFillComponent(const glm::ivec2& seed, int16_t floor, const glm::ivec2& regionMin,
        const glm::ivec2& regionMax, std::vector<uint8_t>& visited, int regionW, int regionH,
        std::vector<glm::ivec2>& outCells)
    {
        static const glm::ivec2 neighbors[4] =
        {
            { 1, 0 },
            { -1, 0 },
            { 0, 1 },
            { 0, -1 }
        };

        std::queue<glm::ivec2> q;
        q.push(seed);

        outCells.push_back(seed);

        while (!q.empty())
        {
            const glm::ivec2 p = q.front();
            q.pop();

            for (const glm::ivec2& offset : neighbors)
            {
                const glm::ivec2 n = p + offset;

                if (n.x < regionMin.x || n.y < regionMin.y || n.x > regionMax.x || n.y > regionMax.y)
                    continue;

                const int lx = n.x - regionMin.x;
                const int ly = n.y - regionMin.y;
                const size_t li = LocalIndex(lx, ly, regionW);

                if (visited[li])
                    continue;

                visited[li] = 1;

                if (!HasStabilityTile(n, floor))
                    continue;

                outCells.push_back(n);
                q.push(n);
            }
        }
    }

    bool TileStabilitySystem::IsTileSupported(const glm::ivec2& cell, int16_t floor) const
    {
        if (!m_scene)
            return true;

        CompactTile stabilityTile;
        const TileDefinitionRegistry& tileDefReg = AssetManager::GetTileDefinitions();

        if (!m_scene->GetCompactTileMap().FindFirstStabilityTile(cell, tileDefReg, floor, stabilityTile))
            return true;

        const TileDefinition* def = tileDefReg.Get(stabilityTile.TypeId);
        if (!def)
            return true;

        const bool isRoof = def->Category == eTileCategory::Roofs;

        if (floor <= 0 && !isRoof)
            return true;

        const int16_t supportFloor = isRoof ? floor : static_cast<int16_t>(floor - 1);

        for (const glm::ivec2& offset : m_cfg.SupportOffsets)
        {
            const glm::ivec2 supportCell = cell + offset;

            if (HasSupportingTile(supportCell, supportFloor))
                return true;
        }

        return false;
    }

    bool TileStabilitySystem::IsRoofTile(const glm::ivec2& cell, int16_t floor) const
    {
        CompactTile tile;
        const TileDefinitionRegistry& defs = AssetManager::GetTileDefinitions();

        if (!m_scene->GetCompactTileMap().FindFirstStabilityTile(cell, defs, floor, tile))
            return false;

        const TileDefinition* def = defs.Get(tile.TypeId);
        if (!def)
            return false;

        return def->Category == eTileCategory::Roofs;
    }

    void TileStabilitySystem::ScheduleCollapse(const std::vector<glm::ivec2>& cells, int16_t floor)
    {
        for (const glm::ivec2& cell : cells)
        {
            bool alreadyPending = false;

            for (const PendingStabilityCollapse& pending : m_pending)
            {
                if (pending.Cell == cell && pending.Floor == floor)
                {
                    alreadyPending = true;
                    break;
                }
            }

            if (alreadyPending)
                continue;

            PendingStabilityCollapse pending;
            pending.Cell = cell;
            pending.Floor = floor;
            pending.Timer = m_cfg.CollapseDelaySeconds;

            m_pending.push_back(pending);
        }
    }

    void TileStabilitySystem::ProcessPendingCollapses(float dt)
    {
        if (!m_scene)
            return;

        for (size_t i = 0; i < m_pending.size();)
        {
            PendingStabilityCollapse& pending = m_pending[i];

            pending.Timer -= dt;

            if (pending.Timer > 0.0f)
            {
                ++i;
                continue;
            }

            if (HasStabilityTile(pending.Cell, pending.Floor) && !IsTileSupported(pending.Cell, pending.Floor))
            {
                CollapseTile(pending.Cell, pending.Floor);

                const int r = m_cfg.DirtyRadiusCells;
                const glm::ivec2 min = pending.Cell - glm::ivec2(r, r);
                const glm::ivec2 max = pending.Cell + glm::ivec2(r, r);

                // Only check things above this collapsed tile.
                MarkDirtyAABB(min, max, static_cast<int16_t>(pending.Floor + 1));
            }

            m_pending[i] = m_pending.back();
            m_pending.pop_back();
        }
    }

    bool TileStabilitySystem::HasSupportingTile(const glm::ivec2& cell, int16_t floor) const
    {
        CompactTileMap& map = m_scene->GetCompactTileMap();
        const TileDefinitionRegistry& tileDefReg = AssetManager::GetTileDefinitions();

        return map.HasSupportingTile(cell, tileDefReg, floor);
    }

    bool TileStabilitySystem::HasStabilityTile(const glm::ivec2& cell, int16_t floor) const
    {
        CompactTileMap& map = m_scene->GetCompactTileMap();

        const TileDefinitionRegistry& tileDefReg = AssetManager::GetTileDefinitions();

        return map.HasStabilityTile(cell, tileDefReg, floor);
    }


    void TileStabilitySystem::CollapseTile(const glm::ivec2& cell, int16_t floor)
    {
        if (!m_scene)
            return;

        Entity owner{};
        size_t tileIndex = SIZE_MAX;
        bool found = false;

        m_scene->ForEach<TransformComponent, TileComponent, IDComponent>(
            [&](Entity e, TransformComponent& transform, TileComponent& tileComp, IDComponent& idComp)
            {
                if (found)
                    return;

                for (size_t i = 0; i < tileComp.tiles.size(); ++i)
                {
                    TileInfo& tile = tileComp.tiles[i];

                    if (tile.floor != floor)
                        continue;

                    glm::vec2 worldPos = glm::vec2(transform.Translation.x, transform.Translation.y) + tile.position;
                    glm::ivec2 tileCell = IsoTileUtils::WorldToIsoCellInt(worldPos);

                    if (tileCell != cell)
                        continue;

                    owner = e;
                    tileIndex = i;
                    found = true;
                    return;
                }
            });

        if (!found || !owner)
            return;

        TransformComponent& ownerXf = owner.GetComponent<TransformComponent>();
        TileComponent& ownerTc = owner.GetComponent<TileComponent>();

        if (tileIndex >= ownerTc.tiles.size())
            return;



        TileTypeKey key;
        key.name = ownerTc.tiles[tileIndex].name;
        key.category = ownerTc.tiles[tileIndex].Category;
        key.direction = ownerTc.tiles[tileIndex].TileDirection;

        uint16_t typeId = 0;
        const TileDefinitionRegistry& tileDefReg = AssetManager::GetTileDefinitions();

        if (tileDefReg.FindTypeId(key, typeId))
        {
            m_scene->GetCompactTileMap().ClearTileFlag(cell, typeId, ownerTc.tiles[tileIndex].floor, CompactTileFlags::CanCollapse);
            m_scene->GetCompactTileMap().ClearTileFlag(cell, typeId, ownerTc.tiles[tileIndex].floor, CompactTileFlags::CanSupport);
            // scene->GetCompactTileMap().ClearTileFlag(cell, typeId, tc.tiles[i].floor, CompactTileFlags::Promoted);
        }


        TileInfo collapsedTile = ownerTc.tiles[tileIndex];

        ownerTc.tiles.erase(ownerTc.tiles.begin() + tileIndex);

        Entity fallingEntity = m_scene->CreateEntity(collapsedTile.name + " Falling");
        IDComponent& fallingId = fallingEntity.GetComponent<IDComponent>();

        TransformComponent& fallingXf = fallingEntity.AddComponent<TransformComponent>();

        glm::vec2 fallingTilePosition = ownerXf.GetVec2Translation();
        //fallingTilePosition.x += collapsedTile.floor;
        fallingTilePosition.y += collapsedTile.floor;
        fallingXf.Set2DTranslation(fallingTilePosition);

        TileComponent& fallingTc = fallingEntity.AddComponent<TileComponent>();

        collapsedTile.IsSpawned = true;
        collapsedTile.IsSupportingRoof = false;
        collapsedTile.floor = 0;

        eTileCategory newCategory = eTileCategory::DynamicObjects;

        collapsedTile.Category = newCategory;
        /*
        collapsedTile.UID = HashUtils::MakeTileUID(
            static_cast<uint64_t>(fallingId.ID),
            collapsedTile.position,
            float(TILE_SIZE),
            static_cast<uint32_t>(newCategory),
            collapsedTile.TileDirection,
            collapsedTile.floor);

        */
        fallingTc.tiles.push_back(collapsedTile);

        const float pxW = float(TILE_SIZE) / float(TILE_PIXEL_WIDTH);
        const float gravityMag = 9.81f;

        float floorHeightWorld = float(TILE_SIZE) * 0.75f;
        float fallHeight = std::max(1.0f, float(floor)) * floorHeightWorld;

        float simulateSeconds = std::sqrt((2.0f * fallHeight) / gravityMag);

        glm::vec2 v0 = glm::vec2(8.0f * pxW, 20.0f * pxW);
        float w0 = glm::radians(90.0f + 30.0f * float(floor));

        PhysicsUtils::AttachSimplePhysics(fallingEntity, v0, w0, simulateSeconds,
            /*destroyOnFinish*/true, { 0.0f, -gravityMag });

        EE_CORE_INFO("TileStabilitySystem collapsed tile '{}' at cell [{}, {}], floor {}",
            collapsedTile.name, cell.x, cell.y, floor);
    }
}