#pragma once

#include "Engine.h"
#include <Engine/Scene/Scene.h>

class PhysicsSystem
{
public:
	static void UpdatePhysicsSystem(float deltaTime, Engine::Scene* scene);


private:
	static float RandomFloat(float minVal, float maxVal);
};

