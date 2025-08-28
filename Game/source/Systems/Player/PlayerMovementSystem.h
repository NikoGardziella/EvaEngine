#pragma once
#include "Engine.h"


class Scene;
class PlayerMovementSystem
{
public:
	static void MovementSystem(float deltaTime, Engine::Scene* scene);
};

