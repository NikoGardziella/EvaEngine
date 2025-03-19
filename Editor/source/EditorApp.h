#pragma once
#include "Engine/Core/Application.h"
#include "Sandbox2D.h"

namespace Engine {

    class Editor : public Application
    {
    public:
        Editor();
        ~Editor();

        Sandbox2D* GetGameLayer();

    private:
        Sandbox2D* m_sandboxLayer;
    };

    Application* CreateApplication();
   

}
