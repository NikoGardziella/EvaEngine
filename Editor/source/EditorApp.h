#pragma once


#include "EditorLayer.h"
#include "Engine/Core/Application.h"

#include "../../Game/source/PixelGame.h"


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
