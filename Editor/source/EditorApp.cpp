#include "Engine.h"
#include "EditorLayer.h"
#include "Engine/Core/EntryPoint.h"



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

	};

	Application* CreateApplication()
	{
		return new Editor();
	} 
}


