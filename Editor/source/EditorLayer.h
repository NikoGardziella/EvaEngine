#pragma once

#include "Engine.h"

//#include "imgui/backends/imgui_impl_vulkan.h"

#include "Panels/SceneHierarchyPanel.h"
#include "Panels/ContentBrowserPanel.h"
#include "Engine/Renderer/EditorCamera.h"
#include "Engine/Platform/Vulkan/VulkanTexture.h"


#include "FPSCounter.h"
#include "AI/json.hpp"
#include <imgui.h>

#include "Panels/DebugPanel.h"
#include "Panels/TileEditorPanel.h"
#include "Panels/EffectsPanel.h"
#include <Engine/Events/KeyEvent.h>
#include <filesystem>
#include <Engine/Scene/Components/Render/TileComponent.h>
#include "Commands/CommandHistory.h"
#include "Commands/CommandGroup.h"
#include "TilePlacement/WallPlacement/WallDirectionalTypeSet.h"
#include "TilePlacement/WallPlacement/WallRectanglePlacementTool.h"
#include <TilePlacement/TerrainPlacement/TerrainRectanglePlacementTool.h>




namespace Engine {

	class Editor;

	class EditorLayer : public Layer
	{
	
		enum class eSceneState
		{
			Edit = 0,
			Play = 1,
			Pause = 2,
		};
		enum class eEditorPlacementTool
		{
			None = 0,
			PaintSingleTile,
			WallRectangle
		};

	public:


		EditorLayer(Editor* editor);
		virtual ~EditorLayer() = default;

		virtual void OnAttach() override;
		virtual void OnDetach() override;
		virtual void OnImGuiRender() override;


		WallDirectionalTypeSet BuildDirectionalWallTypeSetFromSelectedTile();

		void OnUpdate(Timestep timestep) override;
		void PlaceSelectedTile();
		void DrawSelectedTileOutline();
		void OnUpdateECS(Timestep timestep) override;
		void OnEvent(Event& event) override;


		TileInfo OnCreateTileEntity(std::string selectedTileName, glm::vec4 UV, eTileCategory tileCategory);

		CompactTile BuildCompactTileForSelection(Entity selectedEntity, glm::ivec2 isoCell);

		uint64_t GetOrCreatePlacementGroupId(glm::ivec2 originCell);

		//CompactTile PaintCompactTileAtSelection(Entity selectedEntity);

		uint16_t GetOrCreateDefinitionForSelectedTile();


	private:
		bool OnKeyPressed(KeyPressedEvent& e);
		bool OnMouseButtonPressed(MouseButtonPressedEvent& e);

		void DeselectEntity();

		void NewScene();
		void OpenScene();
		void LoadPrefab();
		void OpenScene(const std::filesystem::path& path);
		void SaveSceneAs();
		void SaveScene();

		void SaveAsPrefab();


		// UI panel
		void UI_Toolbar();
		bool DrawRotatedImageButton(ImTextureID texture, const ImVec2& size, float angleRad, const char* id);
		void OnScenePlay();
		void OnSceneStop();
		void OnScenePause();


		void OnDuplicateEntity();
		void SortIsometricTilesByY();

		glm::vec2 GetSnappedIsoPosition();
		bool CanPlaceTile(std::string selectedTileName, glm::ivec2 groundPos);

		void OnOverlayRender();

		uint16_t GetOrCreateDefinitionForTileByName(const std::string& tileNameRaw);

	private:

		Ref<Shader> m_flatColorShader;
		glm::vec4 m_squareColor = { 0.2f, 0.3f, 0.8f, 1.0f };


		Ref<Framebuffer> m_framebuffer;
		glm::vec2 m_viewportSize = { 0.0f, 0.0f};
		std::array<glm::vec2, 2> m_viewportBounds = { glm::vec2(0, 0), glm::vec2(1, 1) };
		glm::vec2 m_viewportOrigin;

		bool m_viewportFocused = false;
		bool m_viewportHovered = false;
		Entity m_selectedEntity;
		glm::vec2 m_selectedTilePosition = { 0.0f, 0.0f };

		std::filesystem::path m_currentScenePath;


		bool m_primaryCamera;
		EditorCamera m_editorCamera;
		bool m_mouseIsInViewPort = false;
		ImVec2 m_localMousePosInViewport;
		Entity m_hoveredEntity;

		Ref<Texture2D> m_checkerBoardTexture;
		Ref<Texture2D> m_spriteSheet;
		Ref<Texture2D> m_textureSpriteSheetPacked;

		Ref<SubTexture2D> m_textureSprite;
		Ref<SubTexture2D> m_textureBarrel;


		std::unordered_map<char, Ref<SubTexture2D>> m_textureMap;


		//panels
		SceneHierarchyPanel m_sceneHierarchyPanel;
		ContentBrowserPanel m_contentBrowserPanel;
		TileEditorPanel m_tileEditorPanel;
		DebugPanel m_debugPanel;
		EffectsPanel m_effectsPanel;

		Ref<Scene> m_editorScene;

		// PlayButton
		eSceneState m_sceneState = eSceneState::Edit;
		Ref<VulkanTexture> m_iconPlay;
		Ref<VulkanTexture> m_iconStop;
		Ref<VulkanTexture> m_iconPause;
		Ref<VulkanTexture> m_iconLoading;

		bool m_showColliders = false;
		bool m_showAreas = false;
		bool m_showGrid = false;

		Ref<Editor> m_editor;

		FPSCounter m_fpsCounter;


		// key shortcuts

		bool m_controlPressed = false;

		// placement tools
		eEditorPlacementTool m_CurrentPlacementTool = eEditorPlacementTool::PaintSingleTile;

		WallRectanglePlacementTool m_WallRectTool;
		TerrainRectanglePlacementTool m_TerrainRectTool;
		
	private:
		

		CommandHistory m_CommandHistory;
		std::unique_ptr<CommandGroup> m_ActiveStroke = nullptr;

		bool m_StrokeCreatedNewEntity = false;
		
	};

}

