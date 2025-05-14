#pragma once

#include "entt.hpp"
#include "Engine.h"



class HealthSystem
{
public:
	static void UpdateHealthSystem(entt::registry& registry, float deltaTime, Engine::Scene* scene);
};

