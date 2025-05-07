#include "pch.h"
#include "Engine/Core/Input.h"

#include <GLFW/glfw3.h>
#include "Engine/Core/Application.h"


namespace Engine {


	bool Input::IsKeyPressed(const KeyCode keycode)
	{
		const auto& window = static_cast<GLFWwindow*>(Application::Get().GetWindow().GetNativeWindow());

		auto state = glfwGetKey(window, keycode);

		return state == GLFW_PRESS;
	}

	

	bool Input::IsMouseButtonPressed(MouseCode keycode)
	{
		const auto& window = static_cast<GLFWwindow*>(Application::Get().GetWindow().GetNativeWindow());

		auto state = glfwGetMouseButton(window, keycode);

		return state == GLFW_PRESS;
	}


	glm::vec2 Input::GetMouseScreenPosition()
	{
		glm::vec2 client = GetMousePosition();

		auto* window = static_cast<GLFWwindow*>(Application::Get().GetWindow().GetNativeWindow());
		int wx, wy;
		glfwGetWindowPos(window, &wx, &wy);

		return { client.x + (float)wx,
					client.y + (float)wy };
	}


	glm::vec2 Input::GetMousePosition()
	{
		auto* window = static_cast<GLFWwindow*>(Application::Get().GetWindow().GetNativeWindow());
		double xpos, ypos;
		glfwGetCursorPos(window, &xpos, &ypos);

		return { (float)xpos, (float)ypos };
	}

	
	

	float Input::GetMouseX()
	{
		const auto& window = static_cast<GLFWwindow*>(Application::Get().GetWindow().GetNativeWindow());
		double xpos, ypos;
		glfwGetCursorPos(window, &xpos, &ypos);
		return (float)xpos;
	}

	float Input::GetMouseY()
	{
		const auto& window = static_cast<GLFWwindow*>(Application::Get().GetWindow().GetNativeWindow());
		double xpos, ypos;
		glfwGetCursorPos(window, &xpos, &ypos);
		return (float)ypos;
	}
	
}
