#include "pch.h"
#include "Engine.h"
#include "EditorLayer.h"
#include "Engine/Core/EntryPoint.h"

#include "Sandbox2D.h"


namespace Engine {


	class Editor : public Application
	{
	public:

		Editor()
			: Application("Eva Editor")
		{
			PushLayer(new EditorLayer());

			

		}

		~Editor()
		{

		}

	public:

	private:
		Sandbox2D* m_sandboxLayer;

	};


	
	Application* CreateApplication()
	{
		return new Editor();
	} 
	
	
}


