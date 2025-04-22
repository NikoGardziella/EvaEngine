#pragma once
#include "Sandbox2D.h"
#include "PixelGame.h"

#include "EditorLayer.h"
#include "Engine/Core/Application.h"

namespace Engine {

    class Editor : public Application
    {
    public:
        Editor();
        ~Editor();

        PixelGame* GetGameLayer();

    private:
        PixelGame* m_gameLayerPtr;
        EditorLayer* m_editorLayerPtr;
    };

    //Application* CreateEditorApplication();
   

}
