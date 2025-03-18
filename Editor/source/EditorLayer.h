#pragma once

#include "Engine.h"

#include "Panels/SceneHierarchyPanel.h"
#include "Panels/ContentBrowserPanel.h"
#include "Engine/Scene/SceneSerializer.h"
#include "Engine/Renderer/EditorCamera.h"

#include "Sandbox2D.h"

namespace Engine {

	class EditorLayer : public Layer
	{
		enum class SceneState
		{
			Edit = 0,
			Play = 1,
		};


	public:

		EditorLayer();
		virtual ~EditorLayer() = default;

		virtual void OnAttach() override;
		virtual void OnDetach() override;
		virtual void OnImGuiRender() override;

		void OnUpdate(Timestep timestep) override;
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
		void OnScenePlay();
		void OnSceneStop();


		void OnDuplicateEntity();

		void OnOverlayRender();

	private:

		OrthographicCameraController m_orthoCameraController;
		Ref<Shader> m_flatColorShader;
		glm::vec4 m_squareColor = { 0.2f, 0.3f, 0.8f, 1.0f };


		Ref<Framebuffer> m_framebuffer;
		glm::vec2 m_viewportSize = { 0.0f, 0.0f};
		glm::vec2 m_viewportBounds[2];
		bool m_viewportFocused = false;
		bool m_viewportHovered = false;

		Ref<Scene> m_activeScene;
		Ref<Scene> m_editorScene;
		Ref<Scene> m_runtimeScene;
		Entity m_squareEntity;

		std::filesystem::path m_currentScenePath;


		Entity m_cameraEntity;
		Entity m_cameraSecondaryEntity;
		bool m_primaryCamera;
		EditorCamera m_editorCamera;
		bool m_mouseIsInViewPort = false;

		Entity m_hoveredEntity;

		Ref<Texture2D> m_checkerBoardTexture;
		Ref<Texture2D> m_spriteSheet;
		Ref<Texture2D> m_textureSpriteSheetPacked;

		Ref<SubTexture2D> m_textureSprite;
		Ref<SubTexture2D> m_textureBarrel;


		uint32_t m_mapWidth;
		uint32_t m_mapHeight;


		std::unordered_map<char, Ref<SubTexture2D>> m_textureMap;


		//panels
		SceneHierarchyPanel m_sceneHierarchyPanel;
		ContentBrowserPanel m_contentBrowserPanel;

		//int m_gizmoType = -1;



		// PlayButton
		SceneState m_sceneState = SceneState::Edit;
		Ref<Texture2D> m_iconPlay;
		Ref<Texture2D> m_iconStop;

		bool m_showColliders = false;

		Ref<Sandbox2D> m_sandbox;
	};

}

