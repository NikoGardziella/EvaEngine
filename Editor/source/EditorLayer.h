#pragma once

#include "Engine.h"

//#include "imgui/backends/imgui_impl_vulkan.h"

#include "Panels/SceneHierarchyPanel.h"
#include "Panels/ContentBrowserPanel.h"
#include "Engine/Renderer/EditorCamera.h"
#include "Engine/Platform/Vulkan/VulkanTexture.h"


#include "FPSCounter.h"
#include "AI/json.hpp"
#include <imgui/imgui.h>
#include "Panels/DebugPanel.h"
#include "Panels/TileEditorPanel.h"

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


	public:


		EditorLayer(Editor* editor);
		virtual ~EditorLayer() = default;

		virtual void OnAttach() override;
		virtual void OnDetach() override;
		virtual void OnImGuiRender() override;


		void OnUpdate(Timestep timestep) override;
		void OnUpdateECS(Timestep timestep) override;
		void OnEvent(Event& event) override;
	private:
		bool OnKeyPressed(KeyPressedEvent& e);
		bool OnMouseButtonPressed(MouseButtonPressedEvent& e);

		void NewScene();
		void OpenScene();
		void OpenScene(const std::filesystem::path& path);
		void SaveSceneAs();
		void SaveScene();


		// UI panel
		void UI_Toolbar();
		bool DrawRotatedImageButton(ImTextureID texture, const ImVec2& size, float angleRad, const char* id);
		void OnScenePlay();
		void OnSceneStop();
		void OnScenePause();


		void OnDuplicateEntity();
		void OnCreateTileEntity(std::string selectedTileName, glm::vec4 UV, eTileCategory tileCategory);

		void OnOverlayRender();

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


		Ref<Scene> m_editorScene;

		// PlayButton
		eSceneState m_sceneState = eSceneState::Edit;
		Ref<VulkanTexture> m_iconPlay;
		Ref<VulkanTexture> m_iconStop;
		Ref<VulkanTexture> m_iconPause;
		Ref<VulkanTexture> m_iconLoading;

		bool m_showColliders = false;

		Ref<Editor> m_editor;

		FPSCounter m_fpsCounter;


		// key shortcuts

		bool m_controlPressed = false;
		
	};

}

