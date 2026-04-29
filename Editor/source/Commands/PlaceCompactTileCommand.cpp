#include "pch.h"
#include "PlaceCompactTileCommand.h"

#include "Engine/Scene/Scene.h"
#include "Engine/AssetManager/AssetManager.h"
#include <Engine/Map/Tile/TileDefinitionRegistry.h>

namespace Engine
{


    PlaceCompactTileCommand::PlaceCompactTileCommand(Scene* scene, const glm::ivec2& worldCell, const CompactTile& newTile,
        const eTileDirection& tileDir)
        : m_Scene(scene), m_WorldCell(worldCell), m_newTile(newTile), m_tileDir(tileDir)
    {
        if (!m_Scene)
            return;

        CompactTile* existing = m_Scene->GetCompactTileMap().FindTile(m_WorldCell, m_newTile.TypeId, m_newTile.Floor);
        if (existing && !existing->IsEmpty())
        {
            m_oldTile = *existing;
            m_HadOldTile = true;
        }
        else
        {
            m_oldTile = CompactTile{};
            m_HadOldTile = false;
        }
    }

    void PlaceCompactTileCommand::Execute()
    {
        if (!m_Scene)
            return;

        if (m_newTile.IsEmpty())
            return;

        CompactTileMap& compactMap = m_Scene->GetCompactTileMap();
        TileDefinitionRegistry& defs = AssetManager::GetTileDefinitions();

        if (compactMap.HasTileType(m_WorldCell, m_newTile.TypeId, m_newTile.Floor))
            return;


        const TileDefinition* newDef = defs.Get(m_newTile.TypeId);
        if (!newDef)
            return;

        if (newDef->Category == eTileCategory::Terrain ||
            newDef->Category == eTileCategory::Roofs)
        {
            compactMap.RemoveTilesByCategory(
                m_WorldCell,
                defs,
                newDef->Category,
                m_newTile.Floor);
        }
        else
        {
            compactMap.RemoveTileByCategoryAndDirection(
                m_WorldCell,
                defs,
                newDef->Category,
                newDef->Direction,
                m_newTile.Floor);
        }

        compactMap.AddTile(m_WorldCell, m_newTile);

        if (m_newTile.GroupId != 0)
            compactMap.RegisterCellForGroup(m_newTile.GroupId, m_WorldCell);

        compactMap.MarkChunkDirtyForCell(m_WorldCell);

        if (m_newTile.GroupId != 0)
        {
            compactMap.ClearPromotionFlagsForGroup(m_newTile.GroupId);
            m_Scene->GetCompactTilePromotion().PromoteGroup(m_Scene, m_newTile.GroupId);
        }

        m_Scene->GetCompactTilePromotion().InvalidateEditorViewportCache();
    }

    void PlaceCompactTileCommand::Undo()
    {
        if (!m_Scene)
            return;

        CompactTileMap& compactMap = m_Scene->GetCompactTileMap();

        CompactTile* existing = compactMap.FindTile(m_WorldCell, m_newTile.TypeId, m_newTile.Floor);
        if (!existing)
            return;

        const uint64_t groupId = existing->GroupId;

        // If this tile is currently promoted, remove its runtime representation too
        if (groupId != 0 &&
            m_Scene->GetCompactTilePromotion().IsGroupPromoted(groupId))
        {
            bool destroyIfLastTile = true;
            m_Scene->GetCompactTilePromotion().RemoveSingleTileFromExistingGroup(
                m_Scene,
                groupId,
                m_WorldCell,
                m_Scene->GetTileManager(),
                destroyIfLastTile);
        }

        compactMap.RemoveTile(m_WorldCell, m_newTile.TypeId);

        // If no more tiles from this group remain in this cell, unregister the cell from the group
        if (groupId != 0)
        {
            const std::vector<CompactTile>* tiles = compactMap.GetTiles(m_WorldCell);
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
            {
                compactMap.RemoveCellFromGroup(groupId, m_WorldCell);
            }
        }

        compactMap.MarkChunkDirtyForCell(m_WorldCell);
    }
}