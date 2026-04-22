#include "pch.h"
#include "TilePlacementUtils.h"
#include "WallRectanglePlacementTool.h"


namespace Engine {


    std::string TilePlacementUtils::RemoveExtension(const std::string& name)
    {
        const size_t dot = name.find_last_of('.');
        if (dot == std::string::npos)
            return name;
        return name.substr(0, dot);
    }


    bool TilePlacementUtils::SplitDirectionalTileName(const std::string& rawName, std::string& outBaseName, char& outDir)
    {
        const std::string name = RemoveExtension(rawName);

        const size_t underscore = name.find_last_of('_');
        if (underscore == std::string::npos)
            return false;

        if (underscore + 1 >= name.size())
            return false;

        outBaseName = name.substr(0, underscore);

        const char dir = name[underscore + 1];
        if (dir != 'N' && dir != 'S' && dir != 'E' && dir != 'W')
            return false;

        outDir = dir;
        return true;
    }

    std::string TilePlacementUtils::MakeDirectionalTileName(const std::string& baseName, char dir)
    {
        return baseName + "_" + std::string(1, dir);
    }

    eTileDirection TilePlacementUtils::GetDirectionForRectCell(eRectCellKind kind)
    {
        switch (kind)
        {
        case eRectCellKind::TopEdge:         return eTileDirection::North;
        case eRectCellKind::BottomEdge:      return eTileDirection::South;
        case eRectCellKind::LeftEdge:        return eTileDirection::West;
        case eRectCellKind::RightEdge:       return eTileDirection::East;

        case eRectCellKind::TopLeftCorner:     return eTileDirection::North;
        case eRectCellKind::TopRightCorner:    return eTileDirection::East;
        case eRectCellKind::BottomLeftCorner:  return eTileDirection::West;
        case eRectCellKind::BottomRightCorner: return eTileDirection::South;

        default: return eTileDirection::South;
        }
    }


    eRectCellKind TilePlacementUtils::ClassifyRectangleCell(const glm::ivec2& cell, const glm::ivec2& minCell,
        const glm::ivec2& maxCell)
    {
        const bool left = cell.x == minCell.x;
        const bool right = cell.x == maxCell.x;
        const bool top = cell.y == minCell.y;
        const bool bottom = cell.y == maxCell.y;

        if (left && top)     return eRectCellKind::TopLeftCorner;
        if (right && top)    return eRectCellKind::TopRightCorner;
        if (left && bottom)  return eRectCellKind::BottomLeftCorner;
        if (right && bottom) return eRectCellKind::BottomRightCorner;

        if (top)    return eRectCellKind::TopEdge;
        if (bottom) return eRectCellKind::BottomEdge;
        if (left)   return eRectCellKind::LeftEdge;
        if (right)  return eRectCellKind::RightEdge;

        return eRectCellKind::Interior;
    }
}
