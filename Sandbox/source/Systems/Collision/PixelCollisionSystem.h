#pragma once
#include "entt.hpp"
#include "Engine.h"

class PixelCollisionSystem
{

public:
	static void UpdatePixelCollisionSystem(entt::registry& registry, float deltaTime, Engine::Scene* scene);

};

