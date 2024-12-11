#pragma once

#include "Core.h"
#include "Engine/Core/Window.h"

#include "Engine/Core/LayerStack.h"
#include "Engine/Events/Event.h"
#include "Engine/Events/ApplicationEvent.h"


namespace Engine
{
	class EE_API Application
	{
	public:
		Application();
		virtual ~Application();
		void Run();

		void PushLayer(Layer* layer);
		void PushOverLay(Layer* layer);

		static Application& Get() { return *s_Instance; }
		Window& GetWindow() { return *m_window; }
	private:

		static Application* s_Instance;

		std::unique_ptr<Window> m_window;
		bool m_isRunning = true;
		
		void OnEvent(Event& e);

		bool OnWindowClose(WindowCloseEvent& e);

		LayerStack m_LayerStack;
	};

	//in client
	Application* CreateApplication();

}


