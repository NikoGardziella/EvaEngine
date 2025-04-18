#pragma once
#include "Engine/Events/MouseCodes.h"

#include "glm/glm.hpp"
#include <Engine/Events/KeyCode.h>

namespace Engine{


	class Input
	{
	public:
		static bool IsKeyPressed(KeyCode key);

		static bool IsMouseButtonPressed(MouseCode button);
		static glm::vec2 GetMousePosition();
		static float GetMouseX();
		static float GetMouseY();
	

	};
}