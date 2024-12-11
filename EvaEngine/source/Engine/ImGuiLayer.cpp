#include "pch.h"
#include "ImGuiLayer.h"
#include "Engine/Core.h"
#include "Core.h"

#include "Engine/Core/Layer.h"

//#include "glad/glad.h"
#include <imgui_internal.h>

#include "Engine/Application.h"

#include "Engine/Platform/OpenGl/imgui_impl_glfw.h"
#include "Engine/Platform/OpenGl/imgui_impl_opengl3.h"
#include "imgui.h"


//remove
#include "GLFW/glfw3.h"
#include "glad/glad.h"

namespace Engine {

		ImGuiLayer::ImGuiLayer() : Layer("ImGuiLayer")
		{

		}

		ImGuiLayer::~ImGuiLayer()
		{

		}

		void ImGuiLayer::OnUpdate()
		{

			ImGuiIO& io = ImGui::GetIO(); (void)io;
			Application& app = Application::Get();
			io.DisplaySize = ImVec2(app.GetWindow().GetWidth(), app.GetWindow().GetHeight());

			float time = (float)glfwGetTime();
			io.DeltaTime = m_Time > 0.0f ? (time - m_Time) : (1.0f / 60.0f);
			m_Time = time;

			ImGui_ImplOpenGL3_NewFrame();
			ImGui_ImplGlfw_NewFrame();
			ImGui::NewFrame();
			
			static bool show = true;
			ImGui::ShowDemoWindow(&show);

			ImGui::Render();
			ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());


		}

		

		void ImGuiLayer::OnAttach()
		{
			// Setup Dear ImGui context
			IMGUI_CHECKVERSION();
			ImGui::CreateContext();
			ImGuiIO& io = ImGui::GetIO(); (void)io;
			io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;       // Enable Keyboard Controls
			io.ConfigFlags |= ImGuiConfigFlags_NavEnableSetMousePos;    
			
			//io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls
			//io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;           // Enable Docking
			//io.ConfigFlags |= ImGuiConfigFlags_ViewportsNoTaskBarIcons;
			//io.ConfigFlags |= ImGuiConfigFlags_ViewportsNoMerge;

			float fontSize = 18.0f;// *2.0f;
			//io.Fonts->AddFontFromFileTTF("assets/fonts/opensans/OpenSans-Bold.ttf", fontSize);
			//io.FontDefault = io.Fonts->AddFontFromFileTTF("assets/fonts/opensans/OpenSans-Regular.ttf", fontSize);

			// Setup Dear ImGui style
			ImGui::StyleColorsDark();
			//ImGui::StyleColorsClassic();

			// When viewports are enabled we tweak WindowRounding/WindowBg so platform windows can look identical to regular ones.
			ImGuiStyle& style = ImGui::GetStyle();
			if (io.ConfigFlags)
			{
				style.WindowRounding = 0.0f;
				style.Colors[ImGuiCol_WindowBg].w = 1.0f;
			}

			//SetDarkThemeColors();

			Application& app = Application::Get();
			GLFWwindow* window = static_cast<GLFWwindow*>(app.GetWindow().GetNativeWindow());

			// Setup Platform/Renderer bindings
			ImGui_ImplGlfw_InitForOpenGL(window, true);
			ImGui_ImplOpenGL3_Init("#version 410") ;

		}

		void ImGuiLayer::OnDetach()
		{
			ImGui_ImplOpenGL3_Shutdown();
			ImGui_ImplGlfw_Shutdown();
			ImGui::DestroyContext();
		}

		void ImGuiLayer::OnEvent(Event& event)
		{
			EventDispatcher dispatcher(event);
			dispatcher.Dispatch<MouseButtonPressedEvent>(EE_BIND_EVENT_FN(ImGuiLayer::OnMouseButtonPressedEvent));
			dispatcher.Dispatch<MouseButtonReleasedEvent>(EE_BIND_EVENT_FN(ImGuiLayer::OnMouseButtonReleasedEvent));
			dispatcher.Dispatch<MouseMovedEvent>(EE_BIND_EVENT_FN(ImGuiLayer::OnMouseMovedEvent));
			dispatcher.Dispatch<MouseScrolledEvent>(EE_BIND_EVENT_FN(ImGuiLayer::OnMouseScrollEvent));

			dispatcher.Dispatch<KeyReleasedEvent>(EE_BIND_EVENT_FN(ImGuiLayer::OnKeyReleasedEvent));
			dispatcher.Dispatch<KeyPressedEvent>(EE_BIND_EVENT_FN(ImGuiLayer::OnKeyPressedEvent));
			dispatcher.Dispatch<WindowResizeEvent>(EE_BIND_EVENT_FN(ImGuiLayer::OnWindowResizedEvent));
	
		}

		bool ImGuiLayer::OnMouseButtonPressedEvent(MouseButtonPressedEvent& e)
		{
			ImGuiIO& io = ImGui::GetIO();
			io.MouseDown[e.GetMouseButton()] = true;

			return false;
		}

		bool ImGuiLayer::OnMouseButtonReleasedEvent(MouseButtonReleasedEvent& e)
		{
			ImGuiIO& io = ImGui::GetIO();
			io.MouseDown[e.GetMouseButton()] = false;

			return false;
		}

		bool ImGuiLayer::OnMouseMovedEvent(MouseMovedEvent& e)
		{
			ImGuiIO& io = ImGui::GetIO();
			io.MousePos = ImVec2(e.GetX(), e.GetY());

			return false;
		}

		bool ImGuiLayer::OnMouseScrollEvent(MouseScrolledEvent& e)
		{
			ImGuiIO& io = ImGui::GetIO();
			io.MouseWheelH += e.GetXOffset();
			io.MouseWheel += e.GetYOffset();

			return false;
		}

		bool ImGuiLayer::OnKeyReleasedEvent(KeyReleasedEvent& e)
		{
			ImGuiIO& io = ImGui::GetIO();

			int key = e.GetKeyCode();
			switch (key)
			{
			case GLFW_KEY_LEFT_CONTROL:
				io.AddKeyEvent(ImGuiKey_LeftCtrl, false);
				break;
			case GLFW_KEY_SPACE:
				io.AddKeyEvent(ImGuiKey_Space, false);
				break;
			case GLFW_KEY_A:
				io.AddKeyEvent(ImGuiKey_A, false);
				break;
			}
			return false;
		}

		bool ImGuiLayer::OnKeyPressedEvent(KeyPressedEvent& e)
		{
			ImGuiIO& io = ImGui::GetIO();

			int key = e.GetKeyCode();
			switch (key)
			{
			case GLFW_KEY_LEFT_CONTROL:
				io.AddKeyEvent(ImGuiKey_LeftCtrl, true);
				break;
			case GLFW_KEY_SPACE:
				io.AddKeyEvent(ImGuiKey_Space, true);
				break;
			case GLFW_KEY_A:
				io.AddKeyEvent(ImGuiKey_A, true);
				break;
			}
			return false;

		}

		bool ImGuiLayer::OnKeyTypedEvent(KeyTypedEvent& e)
		{
			ImGuiIO& io = ImGui::GetIO();
			int key = e.GetKeyCode();

			if (key > 0 && key < 0x10000)
			{
				io.AddInputCharacter((unsigned short)key);
			}

			return false;
		}

		bool ImGuiLayer::OnWindowResizedEvent(WindowResizeEvent& e)
		{
			ImGuiIO& io = ImGui::GetIO();
			io.DisplaySize = ImVec2(e.GetWidth(), e.GetHeight());
			io.DisplayFramebufferScale = ImVec2(1.0f, 1.0f);
			glViewport(0, 0, e.GetWidth(), e.GetHeight());

			return false;
		}
	
}
