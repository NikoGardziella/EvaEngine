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
		//void SetEditorCamera(EditorCamera& camera) { m_editorCamera = camera; }
		void OnImGuiRender();


	

	private:
		bool m_showChunks = false;
		//EditorCamera& m_editorCamera;

		Ref<Scene> m_gameContext;

	};
}


