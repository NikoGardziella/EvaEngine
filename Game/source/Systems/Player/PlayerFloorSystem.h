#pragma once
#include <Engine/Scene/Scene.h>


class Scene;
class PlayerFloorSystem
{
public:
	static void UpdatePlayerFloorSystem(float deltaTime, Engine::Scene* scene);


private:
	static glm::vec2 ClosestPointOnSegment(const glm::vec2& p, const glm::vec2& a, const glm::vec2& b);
	static float GetPointTOnSegment(const glm::vec2& p, const glm::vec2& a, const glm::vec2& b);
};

