#pragma once
#include "Sandbox2D.h"
#include "EditorLayer.h"
#include "Engine/Core/Application.h"

namespace Engine {

    class Editor : public Application
    {
    public:
        Editor();
        ~Editor();

        Sandbox2D* GetGameLayer();

    private:
        Sandbox2D* m_sandboxLayerPtr;
        EditorLayer* m_editorLayerPtr;
    };

    //Application* CreateEditorApplication();
   

}
