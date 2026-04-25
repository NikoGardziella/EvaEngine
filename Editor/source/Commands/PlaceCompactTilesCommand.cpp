#include "pch.h"
#include "PlaceCompactTilesCommand.h"
#include "Engine/Scene/Scene.h"


namespace Engine
{
    PlaceCompactTilesCommand::PlaceCompactTilesCommand(Scene* scene, std::vector<Placement> placements)
        : m_Scene(scene), m_placements(std::move(placements))
    {
    }

    void PlaceCompactTilesCommand::Execute()
    {
        if (!m_Scene)
            return;

        CompactTileMap& compactMap = m_Scene->GetCompactTileMap();
        TileDefinitionRegistry defs = m_Scene->GetTileDefinitions();
        m_executedIndices.clear();
        m_touchedGroups.clear();

        for (size_t i = 0; i < m_placements.size(); i++)
        {
            const Placement& placement = m_placements[i];

            if (placement.NewTile.IsEmpty())
                continue;

            // Skip exact duplicate type already in this cell
            if (compactMap.HasTileType(placement.WorldCell, placement.NewTile.TypeId))
                continue;

            TileDefinitionRegistry& defs = m_Scene->GetTileDefinitions();

            const TileDefinition* newDef = defs.Get(placement.NewTile.TypeId);
            if (!newDef)
                continue;

            if (newDef->Category == eTileCategory::Terrain ||
                newDef->Category == eTileCategory::Roofs)
            {
                compactMap.RemoveTilesByCategory(
                    placement.WorldCell,
                    defs,
                    newDef->Category);
            }
            else
            {
                compactMap.RemoveTileByCategoryAndDirection(
                    placement.WorldCell,
                    defs,
                    newDef->Category,
                    newDef->Direction);
            }


            compactMap.AddTile(placement.WorldCell, placement.NewTile);

            if (placement.NewTile.GroupId != 0)
            {
                compactMap.RegisterCellForGroup(placement.NewTile.GroupId, placement.WorldCell);
                m_touchedGroups.insert(placement.NewTile.GroupId);
            }

           

            compactMap.MarkChunkDirtyForCell(placement.WorldCell);
            m_executedIndices.push_back(i);
        }

        RefreshTouchedGroups();
        m_Scene->GetCompactTilePromotion().InvalidateEditorViewportCache();
    }

    void PlaceCompactTilesCommand::Undo()
    {
        if (!m_Scene)
            return;

        CompactTileMap& compactMap = m_Scene->GetCompactTileMap();
        EE_CORE_WARN("Undo: placements={}, executed={}",
            m_placements.size(), m_executedIndices.size());
        // remove in reverse order
        for (size_t rev = m_executedIndices.size(); rev > 0; rev--)
        {
            const size_t i = m_executedIndices[rev - 1];
            const Placement& placement = m_placements[i];
            EE_CORE_WARN("Undo removing typeId={} at ({}, {})",
                placement.NewTile.TypeId,
                placement.WorldCell.x,
                placement.WorldCell.y);

            CompactTile* existing = compactMap.FindTile(placement.WorldCell, placement.NewTile.TypeId);
            EE_CORE_WARN("existing found = {}", existing ? 1 : 0);
            if (!existing)
                continue;

            const uint64_t groupId = existing->GroupId;
            compactMap.RemoveTile(placement.WorldCell, placement.NewTile.TypeId);

            if (groupId != 0)
            {
                const std::vector<CompactTile>* tiles = compactMap.GetTiles(placement.WorldCell);
                bool stillHasGroupInCell = false;

                if (tiles)
                {
                    for (const CompactTile& t : *tiles)
                    {
                        if (!t.IsEmpty() && t.GroupId == groupId)
                        {
                            stillHasGroupInCell = true;
                            break;
                        }
                    }
                }

                if (!stillHasGroupInCell)
                    compactMap.RemoveCellFromGroup(groupId, placement.WorldCell);

                m_touchedGroups.insert(groupId);
            }

            compactMap.MarkChunkDirtyForCell(placement.WorldCell);
        }

        RefreshTouchedGroupsAfterUndo();
        m_Scene->GetCompactTilePromotion().InvalidateEditorViewportCache();
    }

    void PlaceCompactTilesCommand::RefreshTouchedGroups()
    {
        CompactTileMap& compactMap = m_Scene->GetCompactTileMap();
        auto& promotion = m_Scene->GetCompactTilePromotion();

        for (uint64_t groupId : m_touchedGroups)
        {
            if (groupId == 0)
                continue;

            compactMap.ClearPromotionFlagsForGroup(groupId);
            promotion.PromoteGroup(m_Scene, groupId);
        }
    }

    void PlaceCompactTilesCommand::RefreshTouchedGroupsAfterUndo()
    {
        if (!m_Scene)
            return;

        CompactTileMap& compactMap = m_Scene->GetCompactTileMap();
        auto& promotion = m_Scene->GetCompactTilePromotion();

        for (uint64_t groupId : m_touchedGroups)
        {
            if (groupId == 0)
                continue;

            compactMap.ClearPromotionFlagsForGroup(groupId);

            const std::vector<glm::ivec2>* cells = compactMap.GetCellsForGroup(groupId);
            if (!cells || cells->empty())
            {
                Entity entity{};

                m_Scene->ForEach<IDComponent>(
                    [&](Entity e, IDComponent& id)
                    {
                        if (static_cast<uint64_t>(id.ID) == groupId)
                            entity = e;
                    });

                if (entity)
                    m_Scene->DestroyEntity(entity);

                continue;
            }

            promotion.PromoteGroup(m_Scene, groupId);
        }

        promotion.InvalidateEditorViewportCache();
    }
}