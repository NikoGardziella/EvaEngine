#pragma once
#include "entt.hpp"
#include "Engine.h"


class PlayerCollisionSystem
{
public:
	static void UpdatePlayerCollision(entt::registry& registry, float deltaTime);
};

