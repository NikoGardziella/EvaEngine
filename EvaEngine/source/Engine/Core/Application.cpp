#include "pch.h"
#include "Application.h"
//#include "Engine/Core/Log.h"

#include "Engine/Events/Event.h"
#include "Engine/Events/ApplicationEvent.h"
#include "Engine/Core/Log.h"
#include "Engine/Events/KeyEvent.h"

#include "Engine/Platform/Windows/WindowsWindow.h"

#include "Engine/Core/Input.h"

#include "Engine/Renderer/Renderer.h"

#include "Engine/Renderer/OrthographicCamera.h"
#include "Layer.h"         
#include "LayerStack.h" 

#include "GLFW/glfw3.h" // remove


namespace Engine
{
#define BIND_EVENT_FN(x) std::bind(&Application::x, this, std::placeholders::_1)

	Application* Application::s_Instance = nullptr;

	

	Application::Application(const std::string& name)
	{
		EE_PROFILE_FUNCTION();
		
		s_Instance = this;
		
		m_window = std::unique_ptr<Window>(WindowsWindow::Create(WindowProps(name)));
		m_window->SetEventCallback(BIND_EVENT_FN(OnEvent));

		Renderer::Init();

		m_layerStack = std::make_unique<LayerStack>();

		m_imGuiLayer = std::make_unique<ImGuiLayer>();
		PushLayer(std::move(m_imGuiLayer));
		

	}

	Application::~Application()
	{

	}

	void Application::PushLayer(std::unique_ptr<Layer> layer)
	{
		layer->OnAttach();
		m_layerStack->PushLayer(std::move(layer));
	}

	void Application::PushOverLay(std::unique_ptr<Layer> layer)
	{
		layer->OnAttach();
		m_layerStack->PushOverlay(std::move(layer));

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
				for (const auto& layer : m_layerStack->GetLayers())
				{
					
					layer->OnUpdate(timestep);
					
				}

				EE_PROFILE_SCOPE("Application::Run() - ImGui updates");

			}
			{
				//if (m_LayerStack.GetLayerCount() != 0)
				{
					m_imGuiLayer->Begin();
				
					for (const auto& layer : m_layerStack->GetLayers())
					{
						layer->OnImGuiRender();
					}


					m_imGuiLayer->End();

				}
			}
			if (!m_window)
			{
				EE_ERROR("Failed to create window!");
				return; 
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
		// Access the underlying LayerStack object via *m_LayerStack
		for (auto it = m_layerStack->rbegin(); it != m_layerStack->rend(); ++it)
		{
			if (*it) // Ensure the layer is valid
			{
				// Dispatch the event to the current layer
				(*it)->OnEvent(e);

				// If the event is handled, break out of the loop to stop propagation
				if (e.Handled)
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


