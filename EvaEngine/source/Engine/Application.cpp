#include "pch.h"
#include "Application.h"

#include "Events/Event.h"
#include "Events/ApplicationEvent.h"
#include "Engine/Log.h"
#include "Events/KeyEvent.h"

#include "Engine/Platform/Windows/WindowsWindow.h"

#include "glad/glad.h"

namespace Engine
{
#define BIND_EVENT_FN(x) std::bind(&Application::x, this, std::placeholders::_1)

	Application* Application::s_Instance = nullptr;

	Application::Application()
	{
		m_window = std::unique_ptr<Window>(WindowsWindow::Create());
		m_window->SetEventCallback(BIND_EVENT_FN(OnEvent));

		s_Instance = this;
	}
	Application::~Application()
	{

	}

	void Application::PushLayer(Layer* layer)
	{
		m_LayerStack.PushLayer(layer);
		layer->OnAttach();
	}

	void Application::PushOverLay(Layer* layer)
	{
		m_LayerStack.PushOverlay(layer);
		layer->OnAttach();

	}

	void Application::Run()
	{
		while (m_isRunning)
		{
			glClearColor(1, 0, 1, 1);
			glClear(GL_COLOR_BUFFER_BIT);

			// Swap buffers

			for (Layer* layer : m_LayerStack)
			{
				layer->OnUpdate();
			}
			m_window->OnUpdate();
		}	
	}

	void Application::OnEvent(Event& e)
	{
		EventDispatcher dispatcher(e);
		dispatcher.Dispatch<WindowCloseEvent>(BIND_EVENT_FN(OnWindowClose));

		for (auto it = m_LayerStack.end(); it != m_LayerStack.begin(); )
		{
			(*--it)->OnEvent(e);
			if (e.Handled)
				break;
		}


	}

	bool Application::OnWindowClose(WindowCloseEvent& e)
	{
		m_isRunning = false;
		return true;
	}
}


