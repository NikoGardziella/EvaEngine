#include "pch.h"
#include "WindowsWindow.h"

#include "Engine/Core/Core.h"

#include "Engine/Events/ApplicationEvent.h"
#include "Engine/Events/MouseEvent.h"
#include "Engine/Events/KeyEvent.h"
#include "Engine/Renderer/Renderer.h"
#include "Engine/Core/Log.h"

#include "Engine/Platform/OpenGl/OpenGLContext.h"
#include "Engine/Platform/Vulkan/VulkanContext.h"
#include <backends/imgui_impl_opengl3_loader.h>

namespace Engine {

	static uint8_t s_GLFWWindowCount = 0;

	Window* WindowsWindow::Create(const WindowProps& props)
	{
		return new WindowsWindow(props);
	}


	static void GLFWErrorCallback(int error, const char* description)
	{
		EE_CORE_ERROR("GLFW Error ({}): {}", error, description);
	}

	WindowsWindow::WindowsWindow(const WindowProps& props)
	{
		EE_PROFILE_FUNCTION();

		Init(props);
	}

	WindowsWindow::~WindowsWindow()
	{
		EE_PROFILE_FUNCTION();

		Shutdown();
	}

	void WindowsWindow::Init(const WindowProps& props)
	{
		EE_PROFILE_FUNCTION();

		m_Data.Title = props.Title;
		m_Data.Width = props.Width;
		m_Data.Height = props.Height;

		EE_CORE_INFO("Creating window {} ({}, {})", props.Title, props.Width, props.Height);

		if (s_GLFWWindowCount == 0)
		{
			EE_PROFILE_SCOPE("glfwInit");
			int success = glfwInit();
			glfwSetErrorCallback(GLFWErrorCallback);
		}

		// Configure GLFW window hints based on the selected API
		if (Renderer::GetAPI() == RendererAPI::API::OpenGL)
		{
			//glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
			//glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 5);
			//glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
			//glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE); // GL_TRUE
		}
		else if (Renderer::GetAPI() == RendererAPI::API::Vulkan)
		{
			glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API); // Don't create an OpenGL context
		}

		{
			EE_PROFILE_SCOPE("glfwCreateWindow");

			m_Window = glfwCreateWindow((int)props.Width, (int)props.Height, m_Data.Title.c_str(), nullptr, nullptr);
			++s_GLFWWindowCount;
		}

		// Initialize Graphics Context based on API
		if (Renderer::GetAPI() == RendererAPI::API::OpenGL)
		{
			m_Context = new OpenGLContext(m_Window);
		}
		else if (Renderer::GetAPI() == RendererAPI::API::Vulkan)
		{
			m_Context = new VulkanContext(m_Window);
		}
		m_Context->Init();

		glfwSetWindowUserPointer(m_Window, &m_Data);
		SetVSync(false);


		// Set GLFW callbacks
		glfwSetWindowSizeCallback(m_Window, [](GLFWwindow* window, int width, int height)
			{
				WindowData& data = *(WindowData*)glfwGetWindowUserPointer(window);
				data.Width = width;
				data.Height = height;

				WindowResizeEvent event(width, height);
				data.EventCallback(event);
			});

		glfwSetWindowCloseCallback(m_Window, [](GLFWwindow* window)
			{
				WindowData& data = *(WindowData*)glfwGetWindowUserPointer(window);
				WindowCloseEvent event;
				data.EventCallback(event);
			});

		glfwSetKeyCallback(m_Window, [](GLFWwindow* window, int key, int scancode, int action, int mods)
			{
				WindowData& data = *(WindowData*)glfwGetWindowUserPointer(window);

				switch (action)
				{
				case GLFW_PRESS:
				{
					KeyPressedEvent event(key, 0);
					data.EventCallback(event);
					break;
				}
				case GLFW_RELEASE:
				{
					KeyReleasedEvent event(key);
					data.EventCallback(event);
					break;
				}
				case GLFW_REPEAT:
				{
					KeyPressedEvent event(key, true);
					data.EventCallback(event);
					break;
				}
				}
			});

		glfwSetCharCallback(m_Window, [](GLFWwindow* window, unsigned int keycode)
			{
				WindowData& data = *(WindowData*)glfwGetWindowUserPointer(window);

				KeyTypedEvent event(keycode);
				data.EventCallback(event);
			});

		glfwSetMouseButtonCallback(m_Window, [](GLFWwindow* window, int button, int action, int mods)
			{
				WindowData& data = *(WindowData*)glfwGetWindowUserPointer(window);

				switch (action)
				{
				case GLFW_PRESS:
				{
					MouseButtonPressedEvent event(button);
					data.EventCallback(event);
					break;
				}
				case GLFW_RELEASE:
				{
					MouseButtonReleasedEvent event(button);
					data.EventCallback(event);
					break;
				}
				}
			});

		glfwSetScrollCallback(m_Window, [](GLFWwindow* window, double xOffset, double yOffset)
			{
				WindowData& data = *(WindowData*)glfwGetWindowUserPointer(window);

				MouseScrolledEvent event((float)xOffset, (float)yOffset);
				data.EventCallback(event);
			});

		glfwSetCursorPosCallback(m_Window, [](GLFWwindow* window, double xPos, double yPos)
			{
				WindowData& data = *(WindowData*)glfwGetWindowUserPointer(window);

				MouseMovedEvent event((float)xPos, (float)yPos);
				data.EventCallback(event);
			});
	}

	void WindowsWindow::Shutdown()
	{
		EE_PROFILE_FUNCTION();

		glfwDestroyWindow(m_Window);
		--s_GLFWWindowCount;

		if (s_GLFWWindowCount == 0)
		{
			glfwTerminate();
		}
	}

	void WindowsWindow::OnUpdate()
	{
		EE_PROFILE_FUNCTION();

		glfwPollEvents();

		m_Context->SwapBuffers();
	}

	void WindowsWindow::SetVSync(bool enabled)
	{

		if (enabled)
			glfwSwapInterval(1);
		else
			glfwSwapInterval(0);

		m_Data.VSync = enabled;
	}

	bool WindowsWindow::IsVSync() const
	{
		return m_Data.VSync;
	}
}
