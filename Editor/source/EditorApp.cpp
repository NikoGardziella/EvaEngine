#include "pch.h"
#include "EditorApp.h"
#include "EditorLayer.h"

#include "Engine.h"
#include "EditorLayer.h"
//#include "Engine/Core/EntryPoint.h"

#include "Sandbox2D.h"
#include "PixelGame.h"

namespace Engine {


    Editor::Editor()
        : Application("Eva Editor")
    {
        m_gameLayerPtr = new PixelGame("physics2D");
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


	

