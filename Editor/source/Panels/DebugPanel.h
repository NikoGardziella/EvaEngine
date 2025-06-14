#pragma once

#include <Engine/Scene/Scene.h>

namespace Engine
{

	class DebugPanel
	{

	public:
		DebugPanel() = default;
		~DebugPanel() = default;
		void SetGameContext(const Ref<Scene>& scene);

		void OnImGuiRender();


	


	private:
		Ref<Scene> m_gameContext;

	};
}


