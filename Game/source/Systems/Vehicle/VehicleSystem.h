#pragma once
#include "entt.hpp"
#include "Engine.h"
#include <Engine/Scene/Scene.h>



class VehicleSystem
{
public:
	static void UpdateVehicleSystem(entt::registry& registry, float deltaTime, Engine::Scene* scene);
};

