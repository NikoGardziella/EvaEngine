#pragma once

#include "glm/glm.hpp"

#include <vector>
#include <cstdint>
#include <cstddef>

namespace Engine
{
    class Scene;
    struct TileStabilityConfig
    {
        float CollapseDelaySeconds = 0.15f;
        int DirtyRadiusCells = 2;

        std::vector<glm::ivec2> SupportOffsets =
        {
            { 0, 0 },
            { 1, 0 },
            { -1, 0 },
            { 0, 1 },
            { 0, -1 }
        };
    };

    struct DirtyStabilityAABB
    {
        glm::ivec2 Min{ 0, 0 };
        glm::ivec2 Max{ 0, 0 };
        int16_t Floor = 0;
    };

    struct PendingStabilityCollapse
    {
        glm::ivec2 Cell{ 0, 0 };
        int16_t Floor = 0;
        float Timer = 0.0f;
    };

    class TileStabilitySystem
    {
    public:
        void InitTileStabilitySystem(Scene* scene);

        void Update(float dt);

        void NotifySupportLostAtCell(const glm::ivec2& cell, int16_t floor);
        void MarkDirtyAABB(const glm::ivec2& min, const glm::ivec2& max, int16_t floor);

        TileStabilityConfig& GetConfig() { return m_cfg; }
        const TileStabilityConfig& GetConfig() const { return m_cfg; }

    private:
        void MergeDirtyAABBs();
        void EvaluateDirtyRegionsAndScheduleCollapses();
        void ProcessPendingCollapses(float dt);

        void FloodFillComponent(const glm::ivec2& seed, int16_t floor, const glm::ivec2& regionMin,
            const glm::ivec2& regionMax, std::vector<uint8_t>& visited, int regionW, int regionH,
            std::vector<glm::ivec2>& outCells);

        bool IsTileSupported(const glm::ivec2& cell, int16_t floor) const;
        bool IsRoofTile(const glm::ivec2& cell, int16_t floor) const;
        void ScheduleCollapse(const std::vector<glm::ivec2>& cells, int16_t floor);

        bool HasStabilityTile(const glm::ivec2& cell, int16_t floor) const;
        bool HasSupportingTile(const glm::ivec2& cell, int16_t floor) const;
        void CollapseTile(const glm::ivec2& cell, int16_t floor);

        static size_t LocalIndex(int x, int y, int width)
        {
            return static_cast<size_t>(y) * static_cast<size_t>(width) + static_cast<size_t>(x);
        }

    private:
        Scene* m_scene = nullptr;
        TileStabilityConfig m_cfg;

        std::vector<DirtyStabilityAABB> m_dirty;
        std::vector<DirtyStabilityAABB> m_dirtyMerged;
        std::vector<PendingStabilityCollapse> m_pending;
    };
}