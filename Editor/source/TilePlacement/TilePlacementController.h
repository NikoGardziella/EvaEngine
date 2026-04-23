#pragma once

#include <vector>
#include <string>
#include <glm/glm.hpp>
#include "WallPlacement/WallDirectionalTypeSet.h"
#include "RoofPlacement/RoofDirectionalTypeSet.h"
#include "TerrainPlacement/TerrainRectanglePlacementTool.h"
#include "WallPlacement/WallRectanglePlacementTool.h"
#include "RoofPlacement/RoofRectanglePlacementTool.h"
#include <Commands/CommandGroup.h>
#include <Commands/CommandHistory.h>



namespace Engine
{
    class Scene;
    class SceneHierarchyPanel;
    class TileEditorPanel;
    class EditorCamera;
    class Entity;
    class TilePlacementController
    {
    public:

       
        TilePlacementController(SceneHierarchyPanel& sceneHierarchyPanel, TileEditorPanel& tileEditorPanel,
            Entity& selectedEntity, CommandHistory& commandHistory);

        void HandleInput(bool mouseIsInViewport,
            bool isEditMode,
            bool controlPressed,
            const glm::ivec2& hoveredCell);

        void DrawPreview(bool mouseIsInViewport,
            bool isEditMode);

    private:
        // Placement handlers
        void HandleRoofPlacement(bool controlPressed, const glm::ivec2& hoveredCell);
        void HandleTerrainPlacement(bool controlPressed, const glm::ivec2& hoveredCell);
        void HandleWallPlacement(bool controlPressed, const glm::ivec2& hoveredCell);
        void HandleSingleTilePlacement();

        // Preview
        void DrawSingleTilePreview();
        void DrawTerrainRectanglePreview();
        void DrawWallRectanglePreview();
        void DrawRoofRectanglePreview();
        void DrawPreviewTileByTypeId(uint16_t typeId, const glm::ivec2& cell, const glm::vec4& color);

        bool CanPlaceTile(std::string selectedTileName, glm::ivec2 isoCell);
        CompactTile BuildCompactTileForSelection(Entity selectedEntity, glm::ivec2 isoCell);


        // Helpers
        Ref<Scene> GetEditorScene() const;
        std::string GetSelectedTileName() const;
        eTileCategory GetSelectedTileCategory() const;
        glm::vec4 GetSelectedTileUV() const;

        uint16_t GetOrCreateDefinitionForSelectedTile();
        uint16_t GetOrCreateDefinitionForTileByName(const std::string& tileNameRaw);

        WallDirectionalTypeSet BuildDirectionalWallTypeSetFromSelectedTile();
        RoofDirectionalTypeSet BuildRoofTypeSetFromSelectedTile();

        uint64_t GetOrCreatePlacementGroupId(glm::ivec2 originCell);

    private:

        glm::ivec2 m_hoveredCell;
        SceneHierarchyPanel& m_sceneHierarchyPanel;
        TileEditorPanel& m_tileEditorPanel;
        Entity& m_selectedEntity;

        CommandHistory& m_commandHistory;
        Scope<CommandGroup> m_activeStroke;
        bool m_strokeCreatedNewEntity = false;

        TerrainRectanglePlacementTool m_terrainRectTool;
        WallRectanglePlacementTool m_wallRectTool;
        RoofRectanglePlacementTool m_roofRectTool;
    };
}