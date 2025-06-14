#pragma once
#include "entt.hpp"
#include "Engine.h"

class NPCAIVisionSystem
{
public:

	static void UpdateNPCAIVisionSystem(entt::registry& registry, float deltaTime, Engine::Scene* scene);
};

