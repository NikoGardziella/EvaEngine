#include "pch.h"
#include "TerrainRectanglePlacementTool.h"

#include <algorithm>


#include <Engine/Scene/Scene.h>

namespace Engine
{
    void TerrainRectanglePlacementTool::BeginDrag(const glm::ivec2& startCell)
    {
        if (m_IsDragging)
            return;

        m_StartCell = startCell;
        m_EndCell = startCell;
        m_IsDragging = true;
    }

    void TerrainRectanglePlacementTool::UpdateDrag(const glm::ivec2& currentCell)
    {
        if (!m_IsDragging)
            return;

        m_EndCell = currentCell;
    }

    void TerrainRectanglePlacementTool::CommitDrag(const TerrainRectanglePlacementContext& ctx)
    {
        if (!m_IsDragging)
            return;

        GeneratePlacement(ctx);
        m_IsDragging = false;
    }

    void TerrainRectanglePlacementTool::CancelDrag()
    {
        m_IsDragging = false;
    }

    glm::ivec2 TerrainRectanglePlacementTool::GetMinCell() const
    {
        return {
            std::min(m_StartCell.x, m_EndCell.x),
            std::min(m_StartCell.y, m_EndCell.y)
        };
    }

    glm::ivec2 TerrainRectanglePlacementTool::GetMaxCell() const
    {
        return {
            std::max(m_StartCell.x, m_EndCell.x),
            std::max(m_StartCell.y, m_EndCell.y)
        };
    }

    void TerrainRectanglePlacementTool::BuildPreviewCells(std::vector<glm::ivec2>& outCells) const
    {
        outCells.clear();

        if (!m_IsDragging)
            return;

        const glm::ivec2 minCell = GetMinCell();
        const glm::ivec2 maxCell = GetMaxCell();

        for (int y = minCell.y; y <= maxCell.y; y++)
        {
            for (int x = minCell.x; x <= maxCell.x; x++)
            {
                outCells.push_back({ x, y });
            }
        }
    }

    void TerrainRectanglePlacementTool::GeneratePlacement(const TerrainRectanglePlacementContext& ctx)
    {
        if (!ctx.ActiveScene || !ctx.CompactMap)
            return;

        if (ctx.TypeId == 0)
            return;

        CompactTileMap& compactMap = *ctx.CompactMap;

        const glm::ivec2 minCell = GetMinCell();
        const glm::ivec2 maxCell = GetMaxCell();

        if (ctx.GroupId != 0 && !compactMap.HasGroupInfo(ctx.GroupId))
        {
            compactMap.SetGroupOrigin(ctx.GroupId, minCell);
        }

        for (int y = minCell.y; y <= maxCell.y; y++)
        {
            for (int x = minCell.x; x <= maxCell.x; x++)
            {
                const glm::ivec2 cell{ x, y };

                if (compactMap.HasTileType(cell, ctx.TypeId))
                    continue;

                CompactTile tile{};
                tile.TypeId = ctx.TypeId;
                tile.GroupId = ctx.GroupId;
                tile.Flags = ctx.Flags;
                tile.Aux = ctx.Aux;

                compactMap.AddTile(cell, tile);
            }
        }
    }
}