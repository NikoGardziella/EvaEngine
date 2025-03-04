#pragma once
#include <Engine/Scene/Scene.h>
#include <Engine/Scene/Entity.h>

#include "Engine/Core/Core.h"


namespace Engine {

	class SceneHierarchyPanel
	{

	public:
		SceneHierarchyPanel() = default;
		SceneHierarchyPanel(const Ref<Scene>& context);



		void SetContext(const Ref<Scene>& scene);
		void OnImGuiRender();
		void SceneHierarchyPanel::DrawComponents(Entity entity);

		void SetGizmoType(const int guizmoType) { m_guizmoType = guizmoType; }
		int GetGuizmoType() const { return m_guizmoType; }

		Entity GetSelectedEntity() const { return m_selectionContext;  }
	private:

		void DrawEntityNode(Entity entity);

	private:

		Ref<Scene> m_context;
		Entity m_selectionContext;
		int m_guizmoType = -1;

		friend class Scene;
	};

}


