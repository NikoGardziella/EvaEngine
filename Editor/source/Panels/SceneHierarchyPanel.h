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
	private:

		void DrawEntityNode(Entity entity);

	private:

		Ref<Scene> m_context;
		Entity m_selectionContext;

		friend class Scene;
	};

}


