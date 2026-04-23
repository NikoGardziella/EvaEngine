#include "pch.h"
#include "TilePlacementController.h"

#include <cctype>

#include <Engine/AssetManager/AssetManager.h>
#include <Engine/Core/Input.h>
#include <Engine/Scene/Scene.h>

#include "TilePlacementUtils.h"
#include "Panels/Utils/EditorUtils.h"

#include "Engine/Map/Utils/IsoTileUtils.h"
#include <Panels/SceneHierarchyPanel.h>
#include <Panels/TileEditorPanel.h>
#include <Engine/Scene/Entity.h>
namespace Engine
{
    TilePlacementController::TilePlacementController(SceneHierarchyPanel& sceneHierarchyPanel,
        TileEditorPanel& tileEditorPanel,
        Entity& selectedEntity)
        : m_SceneHierarchyPanel(sceneHierarchyPanel)
        , m_TileEditorPanel(tileEditorPanel)
        , m_SelectedEntity(selectedEntity)
    {
    }

    void TilePlacementController::HandleInput(bool mouseIsInViewport,
        bool isEditMode,
        bool controlPressed,
        const glm::ivec2& hoveredCell)
    {
        if (!mouseIsInViewport)
            return;

        if (!isEditMode)
            return;
        m_hoveredCell = hoveredCell;

        const std::string selectedTileName = GetSelectedTileName();
        if (selectedTileName.empty())
            return;

        const eTileCategory selectedTileCategory = GetSelectedTileCategory();

        if (controlPressed && selectedTileCategory == eTileCategory::Roofs)
        {
            HandleRoofPlacement(controlPressed, hoveredCell);
            return;
        }

        if (controlPressed && selectedTileCategory == eTileCategory::Terrain)
        {
            HandleTerrainPlacement(controlPressed, hoveredCell);
            return;
        }

        if (controlPressed && selectedTileCategory == eTileCategory::Buildings)
        {
            HandleWallPlacement(controlPressed, hoveredCell);
            return;
        }

        HandleSingleTilePlacement();
    }

    void TilePlacementController::DrawPreview(bool mouseIsInViewport, bool isEditMode)
    {
        if (!mouseIsInViewport)
            return;

        if (!isEditMode)
            return;

        const std::string selectedTileName = GetSelectedTileName();
        if (selectedTileName.empty())
            return;

        if (m_RoofRectTool.IsDragging())
        {
            DrawRoofRectanglePreview();
            return;
        }

        if (m_TerrainRectTool.IsDragging())
        {
            DrawTerrainRectanglePreview();
            return;
        }

        if (m_WallRectTool.IsDragging())
        {
            DrawWallRectanglePreview();
            return;
        }

        DrawSingleTilePreview();
    }

    void TilePlacementController::HandleRoofPlacement(bool, const glm::ivec2& hoveredCell)
    {
        if (Input::IsMouseButtonPressed(Mouse::ButtonLeft))
            m_RoofRectTool.BeginDrag(hoveredCell);

        if (m_RoofRectTool.IsDragging() && Input::IsMouseButtonPressed(Mouse::ButtonLeft))
            m_RoofRectTool.UpdateDrag(hoveredCell);

        if (m_RoofRectTool.IsDragging() && Input::IsMouseButtonReleased(Mouse::ButtonLeft))
        {
            m_RoofRectTool.UpdateDrag(hoveredCell);

            RoofRectanglePlacementContext ctx;
            ctx.ActiveScene = GetEditorScene().get();
            ctx.CompactMap = &GetEditorScene()->GetCompactTileMap();

            ctx.TypeSet = BuildRoofTypeSetFromSelectedTile();
            if (!ctx.TypeSet.IsValid())
            {
                EE_CORE_WARN("Invalid roof set");
                m_RoofRectTool.CancelDrag();
                return;
            }

            const glm::ivec2 originCell = m_RoofRectTool.GetMinCell();
            ctx.GroupId = GetOrCreatePlacementGroupId(originCell);
            if (ctx.GroupId == 0)
            {
                m_RoofRectTool.CancelDrag();
                return;
            }

            ctx.Flags = CompactTileFlags::None;
            ctx.Aux = 0;

            m_RoofRectTool.CommitDrag(ctx);
            ctx.ActiveScene->GetCompactTilePromotion().InvalidateEditorViewportCache();

        }
    }

    void TilePlacementController::HandleTerrainPlacement(bool, const glm::ivec2& hoveredCell)
    {
        if (Input::IsMouseButtonPressed(Mouse::ButtonLeft))
            m_TerrainRectTool.BeginDrag(hoveredCell);

        if (m_TerrainRectTool.IsDragging() && Input::IsMouseButtonPressed(Mouse::ButtonLeft))
            m_TerrainRectTool.UpdateDrag(hoveredCell);

        if (m_TerrainRectTool.IsDragging() && Input::IsMouseButtonReleased(Mouse::ButtonLeft))
        {
            m_TerrainRectTool.UpdateDrag(hoveredCell);

            TerrainRectanglePlacementContext ctx;
            ctx.ActiveScene = GetEditorScene().get();
            ctx.CompactMap = &GetEditorScene()->GetCompactTileMap();
            ctx.TypeId = GetOrCreateDefinitionForSelectedTile();

            if (ctx.TypeId == 0)
            {
                EE_CORE_WARN("Terrain rectangle placement aborted: invalid TypeId");
                m_TerrainRectTool.CancelDrag();
                return;
            }

            const glm::ivec2 originCell = m_TerrainRectTool.GetMinCell();
            ctx.GroupId = GetOrCreatePlacementGroupId(originCell);
            if (ctx.GroupId == 0)
            {
                m_TerrainRectTool.CancelDrag();
                return;
            }

            ctx.Flags = CompactTileFlags::None;
            ctx.Aux = 0;

            m_TerrainRectTool.CommitDrag(ctx);
            ctx.ActiveScene->GetCompactTilePromotion().InvalidateEditorViewportCache();

        }
    }

    void TilePlacementController::HandleWallPlacement(bool, const glm::ivec2& hoveredCell)
    {
        if (Input::IsMouseButtonPressed(Mouse::ButtonLeft))
            m_WallRectTool.BeginDrag(hoveredCell);

        if (m_WallRectTool.IsDragging() && Input::IsMouseButtonPressed(Mouse::ButtonLeft))
            m_WallRectTool.UpdateDrag(hoveredCell);

        if (m_WallRectTool.IsDragging() && Input::IsMouseButtonReleased(Mouse::ButtonLeft))
        {
            m_WallRectTool.UpdateDrag(hoveredCell);

            WallRectanglePlacementContext ctx;
            ctx.ActiveScene = GetEditorScene().get();
            ctx.CompactMap = &GetEditorScene()->GetCompactTileMap();

            ctx.DirectionSet = BuildDirectionalWallTypeSetFromSelectedTile();
            if (!ctx.DirectionSet.IsValid())
            {
                EE_CORE_WARN("Invalid wall set");
                m_WallRectTool.CancelDrag();
                return;
            }

            const glm::ivec2 originCell = m_WallRectTool.GetMinCell();
            ctx.GroupId = GetOrCreatePlacementGroupId(originCell);
            if (ctx.GroupId == 0)
            {
                m_WallRectTool.CancelDrag();
                return;
            }

            ctx.WallFlags = CompactTileFlags::None;
            ctx.WallAux = 0;

            m_WallRectTool.CommitDrag(ctx);
            ctx.ActiveScene->GetCompactTilePromotion().InvalidateEditorViewportCache();
        }
    }

    void TilePlacementController::HandleSingleTilePlacement()
    {
        if (Input::IsMouseButtonPressed(Mouse::Button0))
        {
            // keep this in EditorLayer only if you want,
            // but if fully moving out, replace with your single-tile placement implementation
            // Example placeholder:
            // PlaceSelectedTile();
        }
    }

    void TilePlacementController::DrawSingleTilePreview()
    {
        const std::string selectedTile = GetSelectedTileName();
        if (selectedTile.empty())
            return;

        const glm::vec4 uv = m_TileEditorPanel.GetTileUV(selectedTile);
        const glm::vec4 previewColor(0.3f, 1.0f, 0.3f, 0.45f);

        const glm::vec2 ground = IsoTileUtils::IsoToWorldGround(m_hoveredCell);

        VulkanRenderer2D::DrawTile(ground, uv, previewColor);

    }

    void TilePlacementController::DrawTerrainRectanglePreview()
    {
        const std::string selectedTile = GetSelectedTileName();
        if (selectedTile.empty())
            return;

        std::vector<glm::ivec2> previewCells;
        m_TerrainRectTool.BuildPreviewCells(previewCells);

        const glm::vec4 uv = m_TileEditorPanel.GetTileUV(selectedTile);
        const glm::vec4 previewColor(0.3f, 1.0f, 0.3f, 0.45f);

        for (const glm::ivec2& cell : previewCells)
        {
            const glm::vec2 ground = IsoTileUtils::IsoToWorldGround(cell);
            VulkanRenderer2D::DrawTile(ground, uv, previewColor);
        }
    }

    void TilePlacementController::DrawWallRectanglePreview()
    {
        WallDirectionalTypeSet set = BuildDirectionalWallTypeSetFromSelectedTile();
        if (!set.IsValid())
            return;

        const glm::ivec2 minCell = m_WallRectTool.GetMinCell();
        const glm::ivec2 maxCell = m_WallRectTool.GetMaxCell();
        const glm::vec4 previewColor(0.3f, 1.0f, 0.3f, 0.45f);

        for (int y = minCell.y; y <= maxCell.y; y++)
        {
            for (int x = minCell.x; x <= maxCell.x; x++)
            {
                const glm::ivec2 cell{ x, y };
                const eRectCellKind kind = TilePlacementUtils::ClassifyRectangleCell(cell, minCell, maxCell);

                switch (kind)
                {
                case eRectCellKind::Interior:
                    break;

                case eRectCellKind::TopEdge:
                    DrawPreviewTileByTypeId(set.South, cell, previewColor);
                    break;

                case eRectCellKind::BottomEdge:
                    DrawPreviewTileByTypeId(set.North, cell, previewColor);
                    break;

                case eRectCellKind::LeftEdge:
                    DrawPreviewTileByTypeId(set.West, cell, previewColor);
                    break;

                case eRectCellKind::RightEdge:
                    DrawPreviewTileByTypeId(set.East, cell, previewColor);
                    break;

                case eRectCellKind::TopLeftCorner:
                    DrawPreviewTileByTypeId(set.South, cell, previewColor);
                    DrawPreviewTileByTypeId(set.West, cell, previewColor);
                    break;

                case eRectCellKind::TopRightCorner:
                    DrawPreviewTileByTypeId(set.South, cell, previewColor);
                    DrawPreviewTileByTypeId(set.East, cell, previewColor);
                    break;

                case eRectCellKind::BottomLeftCorner:
                    DrawPreviewTileByTypeId(set.North, cell, previewColor);
                    DrawPreviewTileByTypeId(set.West, cell, previewColor);
                    break;

                case eRectCellKind::BottomRightCorner:
                    DrawPreviewTileByTypeId(set.North, cell, previewColor);
                    DrawPreviewTileByTypeId(set.East, cell, previewColor);
                    break;

                default:
                    break;
                }
            }
        }
    }

    void TilePlacementController::DrawRoofRectanglePreview()
    {
        RoofDirectionalTypeSet set = BuildRoofTypeSetFromSelectedTile();
        if (!set.IsValid())
            return;

        const glm::ivec2 minCell = m_RoofRectTool.GetMinCell();
        const glm::ivec2 maxCell = m_RoofRectTool.GetMaxCell();
        const glm::vec4 previewColor(0.3f, 1.0f, 0.3f, 0.45f);

        for (int y = minCell.y; y <= maxCell.y; y++)
        {
            for (int x = minCell.x; x <= maxCell.x; x++)
            {
                const glm::ivec2 cell{ x, y };
                const eRectCellKind kind = TilePlacementUtils::ClassifyRectangleCell(cell, minCell, maxCell);

                uint16_t typeId = 0;

                switch (kind)
                {
                case eRectCellKind::TopLeftCorner:      typeId = set.CornerNorth; break;
                case eRectCellKind::TopRightCorner:     typeId = set.CornerEast;  break;
                case eRectCellKind::BottomLeftCorner:   typeId = set.CornerWest;  break;
                case eRectCellKind::BottomRightCorner:  typeId = set.CornerSouth; break;

                case eRectCellKind::TopEdge:            typeId = set.EdgeSouth;   break;
                case eRectCellKind::BottomEdge:         typeId = set.EdgeNorth;   break;
                case eRectCellKind::LeftEdge:           typeId = set.EdgeWest;    break;
                case eRectCellKind::RightEdge:          typeId = set.EdgeEast;    break;

                case eRectCellKind::Interior:           typeId = set.Fill;        break;

                default:
                    break;
                }

                DrawPreviewTileByTypeId(typeId, cell, previewColor);
            }
        }
    }

    void TilePlacementController::DrawPreviewTileByTypeId(uint16_t typeId, const glm::ivec2& cell, const glm::vec4& color)
    {
        if (typeId == 0)
            return;

        Ref<Scene> scene = GetEditorScene();
        if (!scene)
            return;

        TileDefinitionRegistry& defs = scene->GetTileDefinitions();
        const TileDefinition* def = defs.Get(typeId);
        if (!def)
            return;

        const glm::vec2 ground = IsoTileUtils::IsoToWorldGround(cell);
        VulkanRenderer2D::DrawTile(ground, def->UV, color);
    }

    Ref<Scene> TilePlacementController::GetEditorScene() const
    {
        return m_SceneHierarchyPanel.GetEditorScene();
    }

    std::string TilePlacementController::GetSelectedTileName() const
    {
        return m_TileEditorPanel.GetSelectedTileName();
    }

    eTileCategory TilePlacementController::GetSelectedTileCategory() const
    {
        const std::string selectedTileName = GetSelectedTileName();
        if (selectedTileName.empty())
            return eTileCategory::Terrain;

        const TileProperties& tileProperties = AssetManager::GetTileProperties(selectedTileName);
        return tileProperties.category;
    }

    glm::vec4 TilePlacementController::GetSelectedTileUV() const
    {
        return m_TileEditorPanel.GetTileUV(GetSelectedTileName());
    }

    uint16_t TilePlacementController::GetOrCreateDefinitionForSelectedTile()
    {
        Ref<Scene> scene = GetEditorScene();
        if (!scene)
            return 0;

        TileDefinitionRegistry& defs = scene->GetTileDefinitions();

        const std::string selectedTileName = m_TileEditorPanel.GetSelectedTileName();
        if (selectedTileName.empty())
        {
            EE_CORE_WARN("GetOrCreateDefinitionForSelectedTile: no tile selected");
            return 0;
        }

        const glm::vec4 selectedUV = m_TileEditorPanel.GetTileUV(selectedTileName);
        const TileProperties& tileProps = AssetManager::GetTileProperties(selectedTileName);
        const eTileCategory selectedCategory = tileProps.category;

        TileInfo temp{};
        temp.name = selectedTileName;
        temp.UV = selectedUV;
        temp.Category = selectedCategory;
        temp.TileDirection = EditorUtils::GetDirectionFromTileName(selectedTileName);

        TileTypeKey key = TileManager::MakeTileTypeKey(temp);

        uint16_t existingTypeId = 0;
        if (defs.FindTypeId(key, existingTypeId))
            return existingTypeId;

        TileDefinition def{};
        def.TypeId = defs.GetNextTypeId();
        def.Name = selectedTileName;
        def.UV = selectedUV;
        def.Category = selectedCategory;
        def.Direction = temp.TileDirection;
        def.Material = tileProps.material;
        def.BaseHealth = static_cast<uint16_t>(tileProps.health);
        def.IsDestructible = (selectedCategory == eTileCategory::Buildings);
        def.IsSupportingRoof = (selectedCategory == eTileCategory::Buildings || selectedCategory == eTileCategory::Pillars);
        def.IsRoof = (selectedCategory == eTileCategory::Roofs);

        if (!defs.Register(def, key))
        {
            EE_CORE_WARN("GetOrCreateDefinitionForSelectedTile: failed to register '{}'", selectedTileName);
            return 0;
        }

        return def.TypeId;
    }

    uint16_t TilePlacementController::GetOrCreateDefinitionForTileByName(const std::string& tileNameRaw)
    {
        Ref<Scene> scene = GetEditorScene();
        if (!scene)
            return 0;

        TileDefinitionRegistry& defs = scene->GetTileDefinitions();

        const std::string tileName = TilePlacementUtils::RemoveExtension(tileNameRaw);
        if (tileName.empty())
        {
            EE_CORE_WARN("GetOrCreateDefinitionForTileByName: empty tile name");
            return 0;
        }

        const glm::vec4 uv = m_TileEditorPanel.GetTileUV(tileName);
        const TileProperties& tileProperties = AssetManager::GetTileProperties(tileName);

        const eTileCategory category = tileProperties.category;
        const TileProperties tileProps = tileProperties;

        TileInfo temp{};
        temp.name = tileName;
        temp.UV = uv;
        temp.Category = category;
        temp.TileDirection = EditorUtils::GetDirectionFromTileName(tileName);

        TileTypeKey key = TileManager::MakeTileTypeKey(temp);

        uint16_t existingTypeId = 0;
        if (defs.FindTypeId(key, existingTypeId))
            return existingTypeId;

        TileDefinition def{};
        def.TypeId = defs.GetNextTypeId();
        def.Name = tileName;
        def.UV = uv;
        def.Category = category;
        def.Direction = temp.TileDirection;
        def.Material = tileProps.material;
        def.BaseHealth = static_cast<uint16_t>(tileProps.health);
        def.IsDestructible = (category == eTileCategory::Buildings);
        def.IsSupportingRoof = (category == eTileCategory::Buildings || category == eTileCategory::Pillars);
        def.IsRoof = (category == eTileCategory::Roofs);

        if (!defs.Register(def, key))
        {
            EE_CORE_WARN("GetOrCreateDefinitionForTileByName: failed to register '{}'", tileName);
            return 0;
        }

        return def.TypeId;
    }

    WallDirectionalTypeSet TilePlacementController::BuildDirectionalWallTypeSetFromSelectedTile()
    {
        WallDirectionalTypeSet out{};

        const std::string selectedTileNameRaw = GetSelectedTileName();
        if (selectedTileNameRaw.empty())
        {
            EE_CORE_WARN("BuildDirectionalWallTypeSetFromSelectedTile: no tile selected");
            return out;
        }

        std::string baseName;
        char selectedDir = 0;
        if (!TilePlacementUtils::SplitDirectionalTileName(selectedTileNameRaw, baseName, selectedDir))
        {
            EE_CORE_WARN("BuildDirectionalWallTypeSetFromSelectedTile: '{}' is not directional", selectedTileNameRaw);
            return out;
        }

        out.North = GetOrCreateDefinitionForTileByName(TilePlacementUtils::MakeDirectionalTileName(baseName, 'N'));
        out.South = GetOrCreateDefinitionForTileByName(TilePlacementUtils::MakeDirectionalTileName(baseName, 'S'));
        out.East = GetOrCreateDefinitionForTileByName(TilePlacementUtils::MakeDirectionalTileName(baseName, 'E'));
        out.West = GetOrCreateDefinitionForTileByName(TilePlacementUtils::MakeDirectionalTileName(baseName, 'W'));

        if (!out.IsValid())
        {
            EE_CORE_WARN("BuildDirectionalWallTypeSetFromSelectedTile: missing one or more directional variants for '{}'", baseName);
        }

        return out;
    }

    RoofDirectionalTypeSet TilePlacementController::BuildRoofTypeSetFromSelectedTile()
    {
        RoofDirectionalTypeSet out{};

        const std::string selectedTileNameRaw = GetSelectedTileName();
        if (selectedTileNameRaw.empty())
        {
            EE_CORE_WARN("BuildRoofTypeSetFromSelectedTile: no tile selected");
            return out;
        }

        std::string baseName;
        char selectedDir = 0;
        if (!TilePlacementUtils::SplitDirectionalTileName(selectedTileNameRaw, baseName, selectedDir))
        {
            EE_CORE_WARN("BuildRoofTypeSetFromSelectedTile: '{}' is not directional", selectedTileNameRaw);
            return out;
        }

        size_t suffixStart = baseName.size();
        while (suffixStart > 0 && std::isdigit(static_cast<unsigned char>(baseName[suffixStart - 1])))
        {
            --suffixStart;
        }

        if (suffixStart == baseName.size())
        {
            EE_CORE_WARN("BuildRoofTypeSetFromSelectedTile: could not find numeric suffix in '{}'", baseName);
            return out;
        }

        const std::string familyPrefix = baseName.substr(0, suffixStart);

        out.EdgeNorth = GetOrCreateDefinitionForTileByName(TilePlacementUtils::MakeDirectionalTileName(familyPrefix + "1", 'N'));
        out.EdgeSouth = GetOrCreateDefinitionForTileByName(TilePlacementUtils::MakeDirectionalTileName(familyPrefix + "1", 'S'));
        out.EdgeEast = GetOrCreateDefinitionForTileByName(TilePlacementUtils::MakeDirectionalTileName(familyPrefix + "1", 'E'));
        out.EdgeWest = GetOrCreateDefinitionForTileByName(TilePlacementUtils::MakeDirectionalTileName(familyPrefix + "1", 'W'));

        out.CornerNorth = GetOrCreateDefinitionForTileByName(TilePlacementUtils::MakeDirectionalTileName(familyPrefix + "2", 'N'));
        out.CornerSouth = GetOrCreateDefinitionForTileByName(TilePlacementUtils::MakeDirectionalTileName(familyPrefix + "2", 'S'));
        out.CornerEast = GetOrCreateDefinitionForTileByName(TilePlacementUtils::MakeDirectionalTileName(familyPrefix + "2", 'E'));
        out.CornerWest = GetOrCreateDefinitionForTileByName(TilePlacementUtils::MakeDirectionalTileName(familyPrefix + "2", 'W'));

        out.Fill = GetOrCreateDefinitionForTileByName(TilePlacementUtils::MakeDirectionalTileName(familyPrefix + "3", 'E'));

        if (!out.IsValid())
        {
            EE_CORE_WARN("BuildRoofTypeSetFromSelectedTile: missing one or more roof variants for '{}'", familyPrefix);
        }

        return out;
    }

    uint64_t TilePlacementController::GetOrCreatePlacementGroupId(glm::ivec2 originCell)
    {
        Ref<Scene> scene = GetEditorScene();
        if (!scene)
            return 0;

        Entity selectedEntity = m_SelectedEntity;

        if (m_SceneHierarchyPanel.IsSelectionLocked())
            selectedEntity = m_SceneHierarchyPanel.GetSelectedEntity();

        CompactTileMap& compactMap = scene->GetCompactTileMap();

        if (selectedEntity && scene->IsEntityValid(selectedEntity))
        {
            const uint64_t groupID = static_cast<uint64_t>(selectedEntity.GetUUID());

            if (!compactMap.HasGroupInfo(groupID))
                compactMap.SetGroupOrigin(groupID, originCell);

            return groupID;
        }

        Entity newEntity = scene->CreateEntity("Entity");
        selectedEntity = newEntity;
        m_SelectedEntity = newEntity;
        m_SceneHierarchyPanel.SetSelectedEntity(newEntity);

        const uint64_t groupID = static_cast<uint64_t>(newEntity.GetUUID());

        newEntity.GetComponent<TagComponent>().Tag = "Entity" + std::to_string(groupID);

        if (!newEntity.HasComponent<TransformComponent>())
        {
            TransformComponent& transformComp = newEntity.AddComponent<TransformComponent>();
            const glm::vec2 rootWorldPos = IsoTileUtils::IsoToWorldGround(originCell);
            transformComp.Translation = glm::vec3(rootWorldPos, 0.0f);
        }
        else
        {
            TransformComponent& transformComp = newEntity.GetComponent<TransformComponent>();
            const glm::vec2 rootWorldPos = IsoTileUtils::IsoToWorldGround(originCell);
            transformComp.Translation = glm::vec3(rootWorldPos, 0.0f);
        }

        if (!compactMap.HasGroupInfo(groupID))
            compactMap.SetGroupOrigin(groupID, originCell);

        return groupID;
    }
}