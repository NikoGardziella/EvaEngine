#include "pch.h"
#include "Application.h"

#include "Engine/Events/Event.h"
#include "Engine/Events/ApplicationEvent.h"
#include "Engine/Core/Log.h"
#include "Engine/Events/KeyEvent.h"

#include "Engine/Platform/Windows/WindowsWindow.h"

#include "Engine/Core/Input.h"

#include "Engine/Renderer/Renderer.h"

#include "Engine/Renderer/OrthographicCamera.h"

#include "GLFW/glfw3.h" // remove


namespace Engine
{
#define BIND_EVENT_FN(x) std::bind(&Application::x, this, std::placeholders::_1)

	Application* Application::s_Instance = nullptr;

	

	Application::Application(const std::string& name)
	{
		EE_PROFILE_FUNCTION();
		
		s_Instance = this;
		

		RendererAPI::API selectedAPI = RendererAPI::API::Vulkan;
		RendererAPI::SetRendererAPI(selectedAPI);
		m_window = std::unique_ptr<Window>(WindowsWindow::Create(WindowProps(name)));
		m_window->SetEventCallback(BIND_EVENT_FN(OnEvent));


		m_imGuiLayer = new ImGuiLayer();
		PushLayer(m_imGuiLayer);

		Renderer::Init(selectedAPI);


	}

	Application::~Application()
	{
		PopLayer(m_imGuiLayer);

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

	void Application::PopOverlay(Layer* overlay)
	{
		overlay->OnDetach();
		m_LayerStack.PopOverlay(overlay);
	}

	void Application::PopLayer(Layer* layer)
	{
		layer->OnDetach();
		m_LayerStack.PopLayer(layer);
	}

	void Application::Close()
	{
		m_isRunning = false;
	}

	void Application::Run()
	{
		EE_PROFILE_FUNCTION();

		while (m_isRunning)
		{
			float time = (float)glfwGetTime(); // Move to Platform::GetTime to platform specific
			Timestep timestep = time - m_lastFrameTime;
			m_lastFrameTime = time;
			
			if (!m_minimized)
			{

				EE_PROFILE_SCOPE("Application::Run() - Layer updates");

				// dont update layers if editor is minimized
				for (Layer* layer : m_LayerStack)
				{
					
					layer->OnUpdate(timestep);
					
				}

			}
			
			{
				EE_PROFILE_SCOPE("Application::Run() - ImGui updates");

				m_imGuiLayer->Begin();
				for (Layer* layer : m_LayerStack)
				{
					layer->OnImGuiRender();
				}
				m_imGuiLayer->End();
			}
			

			m_window->OnUpdate();
		}	
	}

	void Application::OnEvent(Event& e)
	{
		EE_PROFILE_FUNCTION();

		// Dispatch the event to specific handlers first
		EventDispatcher dispatcher(e);
		dispatcher.Dispatch<WindowCloseEvent>(BIND_EVENT_FN(Application::OnWindowClose));
		dispatcher.Dispatch<WindowResizeEvent>(BIND_EVENT_FN(Application::OnWindowResize));


		// Propagate the event to layers in reverse order
		for (auto it = m_LayerStack.rbegin(); it != m_LayerStack.rend(); ++it)
		{
			if (*it) // Ensure the layer is valid
			{
				//EE_TRACE("Dispatching event: {}", e.ToString());
				//EE_TRACE("Layer: {}", (*it)->GetName());

				(*it)->OnEvent(e); // Call OnEvent on the current layer
				if (e.Handled)     // Stop propagation if the event is handled
				{

					break;
				}
			}
		}

	}


	bool Application::OnWindowClose(WindowCloseEvent& e)
	{
		m_isRunning = false;
		return true;
	}
	bool Application::OnWindowResize(WindowResizeEvent& e)
	{
		EE_PROFILE_FUNCTION();

		if (e.GetWidth() == 0 || e.GetHeight() == 0)
		{
			m_minimized = true;
			return false;
		}

		m_minimized = false;
		Renderer::OnWindowResize(e.GetWidth(), e.GetHeight());


		return false;
	}
}


