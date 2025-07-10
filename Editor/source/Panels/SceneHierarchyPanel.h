#pragma once

#include <Engine/Scene/Entity.h>
#include "Engine/Core/Core.h"

namespace Engine {

	class Scene;

	class SceneHierarchyPanel
	{
		
	public:
		SceneHierarchyPanel();
		//SceneHierarchyPanel(const Ref<Scene>& context);

		//void SetGameContext(const Ref<Scene>& scene);
		void SetSceneHierarchyPanelScene(const Ref<Scene>& scene);
		Ref<Scene>& GetEditorScene() { return m_sceneHierarchyPanelScene; }
		void OnImGuiRender();
		void DrawComponents(Entity entity);

		void SetGizmoType(const int guizmoType) { m_guizmoType = guizmoType; }
		int GetGuizmoType() const { return m_guizmoType; }

		Entity GetSelectedEntity() const { return m_selectionContext;  }
		void SetSelectedEntity(Entity entity);

		int GetEntityCount() const { return m_entityCount; }
		int GetProjectileCount() const { return m_projectileCount; }
		int GetTileCount() const { return m_tileCount; }

		void DestrtoySelectedEntity(Entity entity);

	private:

		void DrawEntityNode(Entity entity);
		void DrawContext();

	private:

		//Ref<Scene> m_gameContext;
		Ref<Scene> m_sceneHierarchyPanelScene;

		Entity m_selectionContext;
		int m_guizmoType = -1;

		bool m_itemIsClicked = false;
	

		// stats
		int m_entityCount = 0;
		int m_projectileCount = 0;
		int m_tileCount = 0;
		// ****

		friend class Scene;

	};

}


