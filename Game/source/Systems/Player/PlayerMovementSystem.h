#pragma once
#include <Engine/Scene/Scene.h>


class Scene;
class PlayerMovementSystem
{
public:
	static void MovementSystem(float deltaTime, Engine::Scene* scene);
};

