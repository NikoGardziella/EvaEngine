#include "pch.h"
#include "EditorApp.h"
#include "EditorLayer.h"

#include "Engine.h"


// ********************************
// dont remove or you will get:
// unresolved external symbol main referenced in function "int __cdecl
// invoke_main(void)" (?invoke_main@@YAHXZ)
#include "Engine/Core/EntryPoint.h"
// ********************************


namespace Engine {


    Editor::Editor()
        : Application("Eva Editor")
    {
        m_gameLayerPtr = new PixelGame("currentScene");
        PushLayer(m_gameLayerPtr);
        
        m_editorLayerPtr = new EditorLayer(this);
        PushLayer(m_editorLayerPtr);

        
    }

    Editor::~Editor()
    {
        PopLayer(m_gameLayerPtr);
        PopLayer(m_editorLayerPtr);
    }

    PixelGame* Editor::GetGameLayer()
    {
       return m_gameLayerPtr;
    }


    Application* CreateEditorApplication()
    {
        return new Editor();
    }
 
    
};


	

