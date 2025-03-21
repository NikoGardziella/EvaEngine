#pragma once
#include "Engine/Core/Application.h"
#include "Sandbox2D.h"
#include "EditorLayer.h"

namespace Engine {

    class Editor : public Application
    {
    public:
        Editor();
        ~Editor();

        void Init(); // New function to initialize layers after construction
        Sandbox2D* GetGameLayer();

    private:
        Sandbox2D* m_sandboxLayerPtr = nullptr; 
        EditorLayer* m_editorLayerPtr = nullptr;
    };

    Application* CreateApplication();
}

