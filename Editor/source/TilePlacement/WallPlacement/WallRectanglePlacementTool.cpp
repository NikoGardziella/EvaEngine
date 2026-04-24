#include "pch.h"
#include "WallRectanglePlacementTool.h"

#include "Engine/Scene/Scene.h"

#include "TilePlacement/eRectCellKind.h"
#include "TilePlacement/TilePlacementUtils.h"

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

        m_placements.clear();
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

    std::vector<PlaceCompactTilesCommand::Placement> WallRectanglePlacementTool::TakePlacements()
    {
        return std::move(m_placements);
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
                const eRectCellKind kind =
                    TilePlacementUtils::ClassifyRectangleCell(cell, minCell, maxCell);

                switch (kind)
                {
                case eRectCellKind::Interior:
                    break;

                case eRectCellKind::TopEdge:
                    AddWallTileIfNeeded(compactMap, cell, ctx.DirectionSet.South, ctx);
                    break;

                case eRectCellKind::BottomEdge:
                    AddWallTileIfNeeded(compactMap, cell, ctx.DirectionSet.North, ctx);
                    break;

                case eRectCellKind::LeftEdge:
                    AddWallTileIfNeeded(compactMap, cell, ctx.DirectionSet.West, ctx);
                    break;

                case eRectCellKind::RightEdge:
                    AddWallTileIfNeeded(compactMap, cell, ctx.DirectionSet.East, ctx);
                    break;

                case eRectCellKind::TopLeftCorner:
                    AddWallTileIfNeeded(compactMap, cell, ctx.DirectionSet.South, ctx);
                    AddWallTileIfNeeded(compactMap, cell, ctx.DirectionSet.West, ctx);
                    break;

                case eRectCellKind::TopRightCorner:
                    AddWallTileIfNeeded(compactMap, cell, ctx.DirectionSet.South, ctx);
                    AddWallTileIfNeeded(compactMap, cell, ctx.DirectionSet.East, ctx);
                    break;

                case eRectCellKind::BottomLeftCorner:
                    AddWallTileIfNeeded(compactMap, cell, ctx.DirectionSet.North, ctx);
                    AddWallTileIfNeeded(compactMap, cell, ctx.DirectionSet.West, ctx);
                    break;

                case eRectCellKind::BottomRightCorner:
                    AddWallTileIfNeeded(compactMap, cell, ctx.DirectionSet.North, ctx);
                    AddWallTileIfNeeded(compactMap, cell, ctx.DirectionSet.East, ctx);
                    break;

                default:
                    break;
                }
            }
        }
    }

    void WallRectanglePlacementTool::AddWallTileIfNeeded(CompactTileMap& compactMap, const glm::ivec2& cell,
        uint16_t typeId, const WallRectanglePlacementContext& ctx)
    {
        if (typeId == 0)
            return;

        if (compactMap.HasTileType(cell, typeId))
            return;

        CompactTile tile{};
        tile.TypeId = typeId;
        tile.GroupId = ctx.GroupId;
        tile.Flags = ctx.WallFlags;
        tile.Aux = ctx.WallAux;

     //   compactMap.AddTile(cell, tile);

        PlaceCompactTilesCommand::Placement p{};
        p.WorldCell = cell;
        p.NewTile = tile;
        m_placements.push_back(p);
    }


}