#pragma once

#include <vector>
#include <string>
#include <glm/glm.hpp>
#include "WallPlacement/WallDirectionalTypeSet.h"
#include "RoofPlacement/RoofDirectionalTypeSet.h"
#include "TerrainPlacement/TerrainRectanglePlacementTool.h"
#include "WallPlacement/WallRectanglePlacementTool.h"
#include "RoofPlacement/RoofRectanglePlacementTool.h"



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

       
        TilePlacementController(SceneHierarchyPanel& sceneHierarchyPanel,
            TileEditorPanel& tileEditorPanel,
            Entity& selectedEntity);

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
        SceneHierarchyPanel& m_SceneHierarchyPanel;
        TileEditorPanel& m_TileEditorPanel;
        Entity& m_SelectedEntity;

        TerrainRectanglePlacementTool m_TerrainRectTool;
        WallRectanglePlacementTool m_WallRectTool;
        RoofRectanglePlacementTool m_RoofRectTool;
    };
}