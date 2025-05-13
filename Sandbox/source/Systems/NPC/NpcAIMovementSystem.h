#pragma once
#include "entt.hpp"
#include "Engine.h"


class NpcAIMovementSystem
{
public:

	static void UpdateNPCAIMovementSystem(entt::registry& registry, float deltaTime);
};

