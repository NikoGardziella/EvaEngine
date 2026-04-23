#pragma once

#include <vector>
#include <glm/glm.hpp>
#include "RoofRectanglePlacementContext.h"

namespace Engine
{

    class RoofRectanglePlacementTool
    {
    public:
        void BeginDrag(const glm::ivec2& startCell);
        void UpdateDrag(const glm::ivec2& currentCell);
        void CommitDrag(const RoofRectanglePlacementContext& ctx);
        void CancelDrag();

        bool IsDragging() const { return m_IsDragging; }

        glm::ivec2 GetStartCell() const { return m_StartCell; }
        glm::ivec2 GetEndCell() const { return m_EndCell; }

        glm::ivec2 GetMinCell() const;
        glm::ivec2 GetMaxCell() const;

        void BuildPreviewCells(std::vector<glm::ivec2>& outCells) const;

    private:
        void GeneratePlacement(const RoofRectanglePlacementContext& ctx);

    private:
        glm::ivec2 m_StartCell{};
        glm::ivec2 m_EndCell{};
        bool m_IsDragging = false;
    };
}