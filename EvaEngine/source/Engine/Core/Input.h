#pragma once
#include "Engine/Events/MouseCodes.h"

#include "glm/glm.hpp"
#include <Engine/Events/KeyCode.h>

namespace Engine{


	class Input
	{
	public:
		static bool IsKeyPressed(KeyCode key);
		static void Update();
		static bool IsMouseButtonPressed(MouseCode button);
		static bool IsMouseButtonDown(MouseCode keycode);
		static bool IsMouseButtonReleased(MouseCode keycode);
		static glm::vec2 GetMousePosition();
		static glm::vec2 GetMouseScreenPosition();
		static float GetMouseX();
		static float GetMouseY();
	


	private:
		static constexpr int MouseButtonCount = 8;
		static bool s_CurrentMouseButtons[MouseButtonCount + 1];
		static bool s_PreviousMouseButtons[MouseButtonCount + 1];
	};
}