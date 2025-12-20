#pragma once
#include "Engine.h"



class Scene;
class NPCAIVisionSystem
{
public:

	static void UpdateNPCAIVisionSystem(float deltaTime, Engine::Scene* scene);

private:
	static bool NPCAIVisionSystem::HasLOSNow(const Engine::Ref<Engine::GridMap>& grid, const glm::vec3& npcPos3, const glm::vec3& targetPos3, bool debuDraw);

};

