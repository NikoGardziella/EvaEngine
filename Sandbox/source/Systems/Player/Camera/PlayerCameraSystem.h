#pragma once
#include "entt.hpp"
#include "Engine.h"

class PlayerCameraSystem
{
public:

	static void UpdatePlayerCameraSystem(entt::registry& registry, float deltaTime, Engine::Scene* scene);
};

