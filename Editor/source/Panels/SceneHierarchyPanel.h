#pragma once
#include <Engine/Scene/Scene.h>
#include <Engine/Scene/Entity.h>

#include "Engine/Core/Core.h"


namespace Engine {

	class Scene;

	class SceneHierarchyPanel
	{

	public:
		SceneHierarchyPanel() = default;
		SceneHierarchyPanel(const Ref<Scene>& context);



		void SetEditorContext(const Ref<Scene>& scene);
		void SetGameContext(const Ref<Scene>& scene);
		void SetNewComponentsContext(const Ref<Scene>& scene);
		const Ref<Scene>& GetNewComponentsContext() { return m_newComponentsContext; }
		void OnImGuiRender();
		void DrawComponents(Entity entity);

		void SetGizmoType(const int guizmoType) { m_guizmoType = guizmoType; }
		int GetGuizmoType() const { return m_guizmoType; }

		Entity GetSelectedEntity() const { return m_selectionContext;  }
		void SetSelectedEntity(Entity entity);



	private:

		void DrawEntityNode(Entity entity);
		void DrawContext();

	private:

		// remove thus
		Ref<Scene> m_editorContext;
		Ref<Scene> m_gameContext;

		// change this to registry only?
		Ref<Scene> m_newComponentsContext;
		Entity m_selectionContext;
		Entity m_editorSelectionContext;
		int m_guizmoType = -1;

		friend class Scene;
	};

}


