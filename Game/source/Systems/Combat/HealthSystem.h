#pragma once

#include "Engine.h"
#include <Engine/Scene/Scene.h>



class HealthSystem
{
public:
	static void UpdateHealthSystem(float deltaTime, Engine::Scene* scene);
};

