#pragma once
#include "entt.hpp"
#include "Engine.h"

class ProjectileSystem
{
	public: 
		static void UpdateProjectileSystem(entt::registry& registry, float deltaTime, Engine::Scene* scen);
};

