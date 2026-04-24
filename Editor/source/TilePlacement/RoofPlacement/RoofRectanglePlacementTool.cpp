#include "pch.h"
#include "RoofRectanglePlacementTool.h"

#include <algorithm>
#include <Engine/Scene/Scene.h>
#include <TilePlacement/eRectCellKind.h>
#include "TilePlacement/TilePlacementUtils.h"

namespace Engine
{
    void RoofRectanglePlacementTool::BeginDrag(const glm::ivec2& startCell)
    {
        if (m_IsDragging)
            return;

        m_StartCell = startCell;
        m_EndCell = startCell;
        m_IsDragging = true;
    }

    void RoofRectanglePlacementTool::UpdateDrag(const glm::ivec2& currentCell)
    {
        if (!m_IsDragging)
            return;

        m_EndCell = currentCell;
    }

    void RoofRectanglePlacementTool::CommitDrag(const RoofRectanglePlacementContext& ctx)
    {
        if (!m_IsDragging)
            return;

        GeneratePlacement(ctx);
        m_IsDragging = false;
    }

    void RoofRectanglePlacementTool::CancelDrag()
    {
        m_IsDragging = false;
    }

    glm::ivec2 RoofRectanglePlacementTool::GetMinCell() const
    {
        return {
            std::min(m_StartCell.x, m_EndCell.x),
            std::min(m_StartCell.y, m_EndCell.y)
        };
    }

    glm::ivec2 RoofRectanglePlacementTool::GetMaxCell() const
    {
        return {
            std::max(m_StartCell.x, m_EndCell.x),
            std::max(m_StartCell.y, m_EndCell.y)
        };
    }

    void RoofRectanglePlacementTool::BuildPreviewCells(std::vector<glm::ivec2>& outCells) const
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


    std::vector<PlaceCompactTilesCommand::Placement> RoofRectanglePlacementTool::TakePlacements()
    {
        return std::move(m_placements);
    }

    void RoofRectanglePlacementTool::GeneratePlacement(const RoofRectanglePlacementContext& ctx)
    {
        if (!ctx.ActiveScene || !ctx.CompactMap || ctx.GroupId == 0)
            return;

        if (!ctx.TypeSet.IsValid())
            return;

        CompactTileMap& compactMap = *ctx.CompactMap;

        const glm::ivec2 minCell = GetMinCell();
        const glm::ivec2 maxCell = GetMaxCell();




        if (!compactMap.HasGroupInfo(ctx.GroupId))
            compactMap.SetGroupOrigin(ctx.GroupId, minCell);


        if (ctx.TypeSet.FillOnly)
        {
            for (int y = minCell.y; y <= maxCell.y; y++)
            {
                for (int x = minCell.x; x <= maxCell.x; x++)
                {
                    const glm::ivec2 cell{ x, y };

                    CompactTile tile{};
                    tile.TypeId = ctx.TypeSet.Fill;
                    tile.GroupId = ctx.GroupId;
                    tile.Flags = ctx.Flags;
                    tile.Aux = ctx.Aux;

                    //compactMap.AddTile(cell, tile);
                    PlaceCompactTilesCommand::Placement p{};
                    p.WorldCell = cell;
                    p.NewTile = tile;

                    m_placements.push_back(p);
                }
            }

            return;
        }

        for (int y = minCell.y; y <= maxCell.y; y++)
        {
            for (int x = minCell.x; x <= maxCell.x; x++)
            {
                const glm::ivec2 cell{ x, y };
                const eRectCellKind kind = TilePlacementUtils::ClassifyRectangleCell(cell, minCell, maxCell);

                uint16_t typeId = 0;

                switch (kind)
                {
                case eRectCellKind::TopLeftCorner:      typeId = ctx.TypeSet.CornerSouth; break;
                case eRectCellKind::TopRightCorner:     typeId = ctx.TypeSet.CornerEast;  break;
                case eRectCellKind::BottomLeftCorner:   typeId = ctx.TypeSet.CornerWest;  break;
                case eRectCellKind::BottomRightCorner:  typeId = ctx.TypeSet.CornerNorth; break;

                case eRectCellKind::BottomEdge:            typeId = ctx.TypeSet.EdgeNorth;   break;
                case eRectCellKind::TopEdge:         typeId = ctx.TypeSet.EdgeSouth;   break;
                case eRectCellKind::LeftEdge:           typeId = ctx.TypeSet.EdgeWest;    break;
                case eRectCellKind::RightEdge:          typeId = ctx.TypeSet.EdgeEast;    break;

                case eRectCellKind::Interior:           typeId = ctx.TypeSet.Fill;        break;

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
                tile.Flags = ctx.Flags;
                tile.Aux = ctx.Aux;

                //compactMap.AddTile(cell, tile);
                PlaceCompactTilesCommand::Placement p{};
                p.WorldCell = cell;
                p.NewTile = tile;

                m_placements.push_back(p);
            }
        }

        ctx.ActiveScene->GetCompactTilePromotion().InvalidateEditorViewportCache();

    }
}