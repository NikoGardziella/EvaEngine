#pragma once
#include "glm/glm.hpp"

namespace Engine {

	struct PlayerData
	{
		glm::vec2	PlayerPos;
		glm::vec2	PlayerScreenPos;
		glm::vec2	CameraPos;
		float		visionRadiusW;
		float		SceneRadius;
		glm::vec2	screenMin;
		glm::vec2	screenMax;
	};
}