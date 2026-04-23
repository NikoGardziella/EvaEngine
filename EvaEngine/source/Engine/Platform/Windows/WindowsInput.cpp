#include "pch.h"
#include "Engine/Core/Input.h"


#include "Engine/Core/Application.h"


namespace Engine {

	
	bool Engine::Input::s_CurrentMouseButtons[MouseButtonCount + 1] = {};
	bool Engine::Input::s_PreviousMouseButtons[MouseButtonCount + 1] = {};

	void Input::Update()
	{
		const auto& window = static_cast<GLFWwindow*>(Application::Get().GetWindow().GetNativeWindow());

		for (int i = 0; i <= GLFW_MOUSE_BUTTON_LAST; i++)
		{
			s_PreviousMouseButtons[i] = s_CurrentMouseButtons[i];
			s_CurrentMouseButtons[i] = glfwGetMouseButton(window, i) == GLFW_PRESS;
		}
	}

	bool Input::IsKeyPressed(const KeyCode keycode)
	{
		const auto& window = static_cast<GLFWwindow*>(Application::Get().GetWindow().GetNativeWindow());

		auto state = glfwGetKey(window, keycode);

		return state == GLFW_PRESS;
	}

	

	bool Input::IsMouseButtonPressed(MouseCode keycode)
	{
		return s_CurrentMouseButtons[keycode];
	}
	/*
	bool Input::IsMouseButtonDown(MouseCode keycode)
	{
		return s_CurrentMouseButtons[keycode] && !s_PreviousMouseButtons[keycode];
	}
	*/
	bool Input::IsMouseButtonReleased(MouseCode keycode)
	{
		return !s_CurrentMouseButtons[keycode] && s_PreviousMouseButtons[keycode];
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
