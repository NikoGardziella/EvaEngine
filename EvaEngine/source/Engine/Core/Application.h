#pragma once

#include "Core.h"
#include "Engine/Core/Window.h"

#include "Engine/Core/LayerStack.h"
#include "Engine/Events/Event.h"
#include "Engine/Events/ApplicationEvent.h"
#include "Engine/ImGui/ImGuiLayer.h"
#include "Engine/Renderer/Shader.h"
#include "Engine/Renderer/Buffer.h"
#include "Engine/Renderer/VertexArray.h"
#include "Engine/Renderer/OrthographicCamera.h"
#include "Engine/Core/Timestep.h"






namespace Engine
{


	class Application
	{
	public:
		Application(const std::string& name = "");


		virtual ~Application();

		void Run();

		void PushLayer(Layer* layer);
		void PushOverLay(Layer* layer);

		static Application& Get() { return *s_Instance; }
		Window& GetWindow() { return *m_window; }

		void Close();

		ImGuiLayer* GetImGuiLayer() { return m_imGuiLayer; }

	private:

		static Application* s_Instance;

		std::unique_ptr<Window> m_window;
		ImGuiLayer* m_imGuiLayer;

		bool m_isRunning = true;
		bool m_minimized = false;
		
		void OnEvent(Event& e);

		bool OnWindowClose(WindowCloseEvent& e);
		bool OnWindowResize(WindowResizeEvent& e);

		LayerStack m_LayerStack;
		float m_lastFrameTime = 0.0f;


	};

	//in client
	Application* CreateApplication();
}


