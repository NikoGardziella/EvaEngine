#include "pch.h"
#include "WindowsInput.h"

#include <GLFW/glfw3.h>
#include "Engine/Core/Application.h"


namespace Engine {

	// static initialization of interface class. Alternatively in  WindowsWindow::Init
	Input* Input::s_instance = new WindowsInput();

	bool WindowsInput::IsKeyPressedImplentation(int keycode)
	{
		const auto& window = static_cast<GLFWwindow*>(Application::Get().GetWindow().GetNativeWindow());

		auto state = glfwGetKey(window, keycode);

		return state == GLFW_PRESS;
	}

	bool WindowsInput::IsMouseButtonPressedImplentation(int keycode)
	{
		const auto& window = static_cast<GLFWwindow*>(Application::Get().GetWindow().GetNativeWindow());

		auto state = glfwGetMouseButton(window, keycode);

		return state == GLFW_PRESS;
	}

	std::pair<float, float> WindowsInput::GetMousePositionImplentation()
	{
		const auto& window = static_cast<GLFWwindow*>(Application::Get().GetWindow().GetNativeWindow());
		double xpos, ypos;
		glfwGetCursorPos(window, &xpos, &ypos);

		return { (float)xpos, (float)ypos };
	}

	

	float WindowsInput::GetMouseXImplentation()
	{
		const auto& window = static_cast<GLFWwindow*>(Application::Get().GetWindow().GetNativeWindow());
		double xpos, ypos;
		glfwGetCursorPos(window, &xpos, &ypos);
		return (float)xpos;
	}

	float WindowsInput::GetMouseYImplentation()
	{
		const auto& window = static_cast<GLFWwindow*>(Application::Get().GetWindow().GetNativeWindow());
		double xpos, ypos;
		glfwGetCursorPos(window, &xpos, &ypos);
		return (float)ypos;
	}
	
}
