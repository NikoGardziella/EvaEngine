#pragma once

#include "entt.hpp"
#include "Engine.h"

class CharacterControllerSystem
{
public:
	static void UpdateCharacterControllerSystem(entt::registry& registry, float deltaTime);
};

