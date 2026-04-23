#pragma once
#include <vector>
#include <glm/glm.hpp>
#include "WallRectanglePlacementContext.h"
namespace Engine {

    class WallRectanglePlacementTool
    {
        
    public:



    public:
        void BeginDrag(const glm::ivec2 & startCell);
        void UpdateDrag(const glm::ivec2 & currentCell);
        void CommitDrag(const WallRectanglePlacementContext & ctx);
        void CancelDrag();

        bool IsDragging() const { return m_IsDragging; }

        glm::ivec2 GetStartCell() const { return m_StartCell; }
        glm::ivec2 GetEndCell() const { return m_EndCell; }

        glm::ivec2 GetMinCell() const;
        glm::ivec2 GetMaxCell() const;

        void BuildPreviewCells(std::vector<glm::ivec2>&outWallCells,
            std::vector<glm::ivec2>&outFloorCells) const;


    private:
        void GeneratePlacement(const WallRectanglePlacementContext & ctx);
        void AddWallTileIfNeeded(CompactTileMap& compactMap, const glm::ivec2& cell, uint16_t typeId, const WallRectanglePlacementContext& ctx);

    private:
        glm::ivec2 m_StartCell{};
        glm::ivec2 m_EndCell{};
        bool m_IsDragging = false;
    };

}