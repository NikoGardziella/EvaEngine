#pragma once

#include "Core.h"

namespace Engine
{
	class EE_API Application
	{
	public:
		Application();
		virtual ~Application();
		void Run();
	};

	//in client
	Application* CreateApplication();

}


