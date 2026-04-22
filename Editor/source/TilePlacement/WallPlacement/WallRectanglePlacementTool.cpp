#include "pch.h"
#include "WallRectanglePlacementTool.h"

#include "Engine/Scene/Scene.h"
#include "../eRectCellKind.h"
#include "../TilePlacementUtils.h"


namespace Engine
{
    void WallRectanglePlacementTool::BeginDrag(const glm::ivec2& startCell)
    {
        if (m_IsDragging)
            return;

        m_StartCell = startCell;
        m_EndCell = startCell;
        m_IsDragging = true;
    }

    void WallRectanglePlacementTool::UpdateDrag(const glm::ivec2& currentCell)
    {
        if (!m_IsDragging)
            return;

        m_EndCell = currentCell;
    }

    void WallRectanglePlacementTool::CommitDrag(const WallRectanglePlacementContext& ctx)
    {
        if (!m_IsDragging)
            return;

        GeneratePlacement(ctx);
        m_IsDragging = false;
    }

    void WallRectanglePlacementTool::CancelDrag()
    {
        m_IsDragging = false;
    }

    glm::ivec2 WallRectanglePlacementTool::GetMinCell() const
    {
        return {
            std::min(m_StartCell.x, m_EndCell.x),
            std::min(m_StartCell.y, m_EndCell.y)
        };
    }

    glm::ivec2 WallRectanglePlacementTool::GetMaxCell() const
    {
        return {
            std::max(m_StartCell.x, m_EndCell.x),
            std::max(m_StartCell.y, m_EndCell.y)
        };
    }

    void WallRectanglePlacementTool::BuildPreviewCells(std::vector<glm::ivec2>& outWallCells,
        std::vector<glm::ivec2>& outFloorCells) const
    {
        outWallCells.clear();
        outFloorCells.clear();

        if (!m_IsDragging)
            return;

        const glm::ivec2 minCell = GetMinCell();
        const glm::ivec2 maxCell = GetMaxCell();

        for (int y = minCell.y; y <= maxCell.y; y++)
        {
            for (int x = minCell.x; x <= maxCell.x; x++)
            {
                glm::ivec2 cell{ x, y };
                const eRectCellKind kind = TilePlacementUtils::ClassifyRectangleCell(cell, minCell, maxCell);

                if (kind == eRectCellKind::Interior)
                    outFloorCells.push_back(cell);
                else
                    outWallCells.push_back(cell);
            }
        }
    }

    void WallRectanglePlacementTool::GeneratePlacement(const WallRectanglePlacementContext& ctx)
    {
        if (!ctx.ActiveScene || !ctx.CompactMap || ctx.GroupId == 0)
            return;

        CompactTileMap& compactMap = *ctx.CompactMap;

        const glm::ivec2 minCell = GetMinCell();
        const glm::ivec2 maxCell = GetMaxCell();

        if (!compactMap.HasGroupInfo(ctx.GroupId))
            compactMap.SetGroupOrigin(ctx.GroupId, minCell);

        for (int y = minCell.y; y <= maxCell.y; y++)
        {
            for (int x = minCell.x; x <= maxCell.x; x++)
            {
                const glm::ivec2 cell{ x, y };
                const eRectCellKind kind = TilePlacementUtils::ClassifyRectangleCell(cell, minCell, maxCell);

                if (kind == eRectCellKind::Interior)
                    continue;

                uint16_t typeId = 0;

                switch (kind)
                {
                case eRectCellKind::TopEdge:            typeId = ctx.DirectionSet.North; break;
                case eRectCellKind::BottomEdge:         typeId = ctx.DirectionSet.South; break;
                case eRectCellKind::LeftEdge:           typeId = ctx.DirectionSet.West;  break;
                case eRectCellKind::RightEdge:          typeId = ctx.DirectionSet.East;  break;

                case eRectCellKind::TopLeftCorner:      typeId = ctx.DirectionSet.North; break;
                case eRectCellKind::TopRightCorner:     typeId = ctx.DirectionSet.North; break;
                case eRectCellKind::BottomLeftCorner:   typeId = ctx.DirectionSet.South; break;
                case eRectCellKind::BottomRightCorner:  typeId = ctx.DirectionSet.South; break;

                default:
                    continue;
                }

                if (typeId == 0)
                    continue;

                if (compactMap.HasTileType(cell, typeId))
                    continue;

                CompactTile tile{};
                tile.TypeId = typeId;
                tile.GroupId = ctx.GroupId;
                tile.Flags = ctx.WallFlags;
                tile.Aux = ctx.WallAux;
                EE_CORE_INFO("adding to cell {}", cell);
                compactMap.AddTile(cell, tile);
            }
        }
        //ctx.ActiveScene->GetCompactTilePromotion().PromoteGroup(ctx.ActiveScene, ctx.GroupId);
    }
}