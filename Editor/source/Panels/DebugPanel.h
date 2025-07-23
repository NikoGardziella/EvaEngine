#pragma once

#include <Engine/Scene/Scene.h>



namespace Engine
{
	class Editor;
	class DebugPanel
	{

	public:
		DebugPanel() = default;
		~DebugPanel() = default;

		void SetGameContext(const Ref<Scene>& scene);
		void SetEditor(const Ref<Editor>& editor);

		//void SetEditorCamera(EditorCamera& camera) { m_editorCamera = camera; }
		void OnImGuiRender();


	

	private:
		bool m_showChunks = false;
		bool m_showGrid = false;
		bool m_showLOS = false;

		//EditorCamera& m_editorCamera;

		Ref<Scene> m_gameContext;
		Ref<Editor> m_editor;
	};
}


