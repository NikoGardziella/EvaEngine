#pragma once
#include "entt.hpp"
#include "Engine.h"

class PlayerMovementSystem
{
public:
	static void MovementSystem(entt::registry& registry, float deltaTime, Engine::Scene* scene);
};

