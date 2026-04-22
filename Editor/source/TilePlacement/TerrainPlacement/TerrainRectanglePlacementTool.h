#pragma once

#include <vector>
#include <glm/glm.hpp>

namespace Engine
{
    class Scene;
    class CompactTileMap;

    struct TerrainRectanglePlacementContext
    {
        Scene* ActiveScene = nullptr;
        CompactTileMap* CompactMap = nullptr;

        uint16_t TypeId = 0;
        uint64_t GroupId = 0;

        uint8_t Flags = 0;
        uint8_t Aux = 0;
    };

    class TerrainRectanglePlacementTool
    {
    public:
        void BeginDrag(const glm::ivec2& startCell);
        void UpdateDrag(const glm::ivec2& currentCell);
        void CommitDrag(const TerrainRectanglePlacementContext& ctx);
        void CancelDrag();

        bool IsDragging() const { return m_IsDragging; }

        glm::ivec2 GetStartCell() const { return m_StartCell; }
        glm::ivec2 GetEndCell() const { return m_EndCell; }

        glm::ivec2 GetMinCell() const;
        glm::ivec2 GetMaxCell() const;

        void BuildPreviewCells(std::vector<glm::ivec2>& outCells) const;

    private:
        void GeneratePlacement(const TerrainRectanglePlacementContext& ctx);

    private:
        glm::ivec2 m_StartCell{};
        glm::ivec2 m_EndCell{};
        bool m_IsDragging = false;
    };
}