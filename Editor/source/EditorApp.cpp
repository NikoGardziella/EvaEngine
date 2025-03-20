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
        PushLayer(new EditorLayer(this));

        m_sandboxLayer = new Sandbox2D();       
        PushLayer(m_sandboxLayer);
        
        
    }

    Editor::~Editor()
    {
    }

    Sandbox2D* Editor::GetGameLayer()
    {
        return m_sandboxLayer;
    }


    Application* CreateApplication()
    {
        return new Editor();
    }
	
};


	

