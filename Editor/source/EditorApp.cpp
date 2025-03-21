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
        // Do not push layers inside the constructor
    }

    void Editor::Init()
    {
        auto sandbox = std::make_unique<Sandbox2D>();
        m_sandboxLayerPtr = sandbox.get();  // Store a raw pointer
        PushLayer(std::move(sandbox));

        auto editorLayer = std::make_unique<EditorLayer>(this);
        PushLayer(std::move(editorLayer));
    }


    Editor::~Editor()
    {
    }

    Sandbox2D* Editor::GetGameLayer()
    {
        return m_sandboxLayerPtr;
    }

    Application* CreateApplication()
    {
        auto editor = new Editor();
        editor->Init();
        return editor;
    }
};



	

