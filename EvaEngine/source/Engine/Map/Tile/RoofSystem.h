#pragma once

#include <glm/glm.hpp>
#include <vector>
#include "SceneRoofTileAccess.h"
#include <glm/fwd.hpp>
#include <Engine/Core/Config.h>


namespace Engine {

    constexpr float ROOF_STEP_X = 0.5f;
    constexpr float ROOF_STEP_Y = 0.25f;


    struct RoofSystemConfig
    {
        int   minSupportsPerComponent = 1;
        int   dirtyRadiusOnSupportChange = 2;
        int   dirtyRadiusOnRoofChange = 2;
        float collapseDelaySeconds = 0.40f;
        bool  use8Connected = false;
        int roofSupportYOffsetCells = 4;
        int supportSearchRadiusCells = 2;
        int minSupportsPerRoofTile = 1;
    };
    struct DirtyAABB
    {
        glm::ivec2 min;
        glm::ivec2 max;
    };

    struct PendingCollapse
    {
        std::vector<glm::ivec2> tiles;
        float timeLeft = 0.0f;
    };

    struct RoofSystemState
    {
        std::vector<DirtyAABB> dirty;
        std::vector<DirtyAABB> dirtyMerged;
        std::vector<PendingCollapse> pending;
    };

    class Scene;
    class RoofSystem
    {
    private:
        


    public:
        void Init(Scene* scene, SceneRoofTileAccess* access, const RoofSystemConfig& cfg);
        void Update(float dt);

        void NotifySupportLostAt(const glm::ivec2& supportPos);
        void NotifySupportLostAtWorld(const glm::vec2& supportWorld);
        void NotifySupportAddedAt(const glm::ivec2& supportPos);

    private:
        RoofSystemState& StateFor(Engine::RoofSystem* self);
        void FloodFillRoofComponent(const SceneRoofTileAccess* access, const RoofSystemConfig& cfg, const glm::ivec2& seed, const glm::ivec2& regionMin, const glm::ivec2& regionMax, std::vector<uint8_t>& visited, int regionW, int regionH, std::vector<glm::ivec2>& outTiles);
        bool PendingAlreadyContainsAny(const RoofSystemState& st, const std::vector<glm::ivec2>& tiles);
        void ScheduleCollapse(RoofSystemState& st, const RoofSystemConfig& cfg, const std::vector<glm::ivec2>& tiles);
        bool IsRoofCellSupported(const SceneRoofTileAccess* access, const RoofSystemConfig& cfg, const glm::ivec2& roofCell);
        void EvaluateDirtyRegionsAndScheduleCollapses(RoofSystemState& st, const SceneRoofTileAccess* access, const RoofSystemConfig& cfg);
        void ProcessPendingCollapses(RoofSystemState& st, SceneRoofTileAccess* access, const RoofSystemConfig& cfg, float dt);


    private:
        Scene* m_scene = nullptr;
        SceneRoofTileAccess* m_access = nullptr;
        RoofSystemConfig m_cfg{};

        
    };

} 