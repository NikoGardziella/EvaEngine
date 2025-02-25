#pragma once

#include "Engine.h"

#include "Panels/SceneHierarchyPanel.h"

namespace Engine {

	class EditorLayer : public Layer
	{
	public:

		EditorLayer();
		virtual ~EditorLayer() = default;

		virtual void OnAttach() override;
		virtual void OnDetach() override;
		virtual void OnImGuiRender() override;

		void OnUpdate(Timestep timestep) override;
		void OnEvent(Event& event) override;

	private:

		OrthographicCameraController m_orthoCameraController;
		Ref<Shader> m_flatColorShader;
		glm::vec4 m_squareColor = { 0.2f, 0.3f, 0.8f, 1.0f };


		Ref<Framebuffer> m_framebuffer;
		glm::vec2 m_viewportSize = { 0.0f, 0.0f};
		bool m_viewportFocused = false;
		bool m_viewportHovered = false;

		Ref<Scene> m_activeScene;
		Entity m_squareEntity;

		Entity m_cameraEntity;
		Entity m_cameraSecondaryEntity;
		bool m_primaryCamera;


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


	};

}

