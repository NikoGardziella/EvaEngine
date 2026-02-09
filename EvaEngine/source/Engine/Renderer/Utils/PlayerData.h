#pragma once
#include "glm/glm.hpp"

namespace Engine {

	struct PlayerData
	{
		glm::vec2	PlayerPos;
		glm::vec2	CameraPos;
		float		visionRadiusW;
		float		SceneRadius;
	};
}