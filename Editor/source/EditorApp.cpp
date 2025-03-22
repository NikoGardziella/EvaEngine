#include "pch.h"
#include "EditorApp.h"
#include "EditorLayer.h"
#include "Sandbox2D.h"
#include "Engine.h"
#include "EditorLayer.h"
#include "Engine/Core/EntryPoint.h"



namespace Engine {


    Editor::Editor()
        : Application("Eva Editor")
    {
        m_editorLayerPtr = new EditorLayer(this);
        PushLayer(m_editorLayerPtr);

        m_sandboxLayerPtr = new Sandbox2D();       
        PushLayer(m_sandboxLayerPtr);
        
        
    }

    Editor::~Editor()
    {
        PopLayer(m_sandboxLayerPtr);
        PopLayer(m_editorLayerPtr);
    }

    Sandbox2D* Editor::GetGameLayer()
    {
        return m_sandboxLayerPtr;
    }


    Application* CreateApplication()
    {
        return new Editor();
    }
	
};


	

