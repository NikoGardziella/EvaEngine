#include "pch.h"
<<<<<<< HEAD
#include "EditorApp.h"
#include "EditorLayer.h"
#include "Sandbox2D.h"
=======
#include "Engine.h"
#include "EditorLayer.h"
#include "Engine/Core/EntryPoint.h"

#include "Sandbox2D.h"

>>>>>>> ff0c3b600b617aa742d76fd6bff3b49a5a8e1cde

namespace Engine {


    Editor::Editor()
        : Application("Eva Editor")
    {
        PushLayer(new EditorLayer(this));

<<<<<<< HEAD
        m_sandboxLayer = new Sandbox2D();
        PushLayer(m_sandboxLayer);
    }
=======
		Editor()
			: Application("Eva Editor")
		{
			PushLayer(new EditorLayer());

			

		}
>>>>>>> ff0c3b600b617aa742d76fd6bff3b49a5a8e1cde

    Editor::~Editor()
    {
        delete m_sandboxLayer;
    }

    Sandbox2D* Editor::GetGameLayer()
    {
        return m_sandboxLayer;
    }

<<<<<<< HEAD
    Application* CreateApplication()
    {
        return new Editor();
    }

=======
	public:

	private:
		Sandbox2D* m_sandboxLayer;

	};


	
	Application* CreateApplication()
	{
		return new Editor();
	} 
	
	
>>>>>>> ff0c3b600b617aa742d76fd6bff3b49a5a8e1cde
}
