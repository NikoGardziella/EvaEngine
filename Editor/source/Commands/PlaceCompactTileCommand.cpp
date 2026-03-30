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

        CompactTile& tile = m_Scene->GetCompactTileMap().GetOrCreateTile(m_WorldCell);

        if (m_HadOldTile)
        {
            tile = m_OldTile;
        }
        else
        {
            // Revert to empty tile
            tile = CompactTile{};
        }
    }
}