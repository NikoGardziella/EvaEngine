#include "pch.h"
#include "PlaceCompactTileCommand.h"

#include "Engine/Scene/Scene.h"

namespace Engine
{
    PlaceCompactTileCommand::PlaceCompactTileCommand(Scene* scene, const glm::ivec2& worldCell, const CompactTile& newTile)
        : m_Scene(scene), m_WorldCell(worldCell), m_NewTile(newTile)
    {
        if (!m_Scene)
            return;

        CompactTile* existing = m_Scene->GetCompactTileMap().GetTile(m_WorldCell);
        if (existing)
        {
            m_OldTile = *existing;
            m_HadOldTile = true;
        }
    }

    void PlaceCompactTileCommand::Execute()
    {
        
    }

    void PlaceCompactTileCommand::Undo()
    {
        if (!m_Scene)
            return;

        CompactTileMap& compactMap = m_Scene->GetCompactTileMap();

        // Current tile at the cell (the one placed by Execute)
        CompactTile* currentTile = compactMap.GetTile(m_WorldCell);
        uint64_t currentGroupId = 0;

        if (currentTile && !currentTile->IsEmpty())
            currentGroupId = currentTile->GroupId;

        // If this tile was promoted into a runtime entity, remove it there too
        if (currentGroupId != 0 && m_Scene->GetCompactTilePromotion().IsGroupPromoted(currentGroupId))
        {
      
            
            m_Scene->GetCompactTilePromotion().RemoveSingleTileFromExistingGroup(
                m_Scene,
                currentGroupId,
                m_WorldCell,
                m_Scene->GetTileManager());
        }

        // Remove current group registration if needed
        if (currentGroupId != 0)
        {
            compactMap.RemoveCellFromGroup(currentGroupId, m_WorldCell);
        }

        CompactTile& tile = compactMap.GetOrCreateTile(m_WorldCell);

        if (m_HadOldTile)
        {
            tile = m_OldTile;

            if (!tile.IsEmpty() && tile.GroupId != 0)
            {
                compactMap.RegisterCellForGroup(tile.GroupId, m_WorldCell);

                // If old tile belonged to an already-promoted group, put it back there too
                if (m_Scene->GetCompactTilePromotion().IsGroupPromoted(tile.GroupId))
                {
                    m_Scene->GetCompactTilePromotion().PromoteSingleTileIntoExistingGroup(
                        m_Scene,
                        tile.GroupId,
                        m_WorldCell,
                        m_Scene->GetTileManager());
                }
            }
        }
        else
        {
            tile = CompactTile{};
        }

        TileChunk& chunk = compactMap.GetOrCreateChunk(WorldCellToChunkCoord(m_WorldCell));
        chunk.DrawCacheDirty = true;
    }
}