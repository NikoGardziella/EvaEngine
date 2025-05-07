#pragma once

#include "entt.hpp"
#include "Engine.h"

class CharacterControllerSystem
{
public:
	static void UpdateCharacterControllerSystem(entt::registry& registry, float deltaTime);

private:

	static void ShootProjectile(entt::registry& registry, entt::entity entity, const glm::vec2& position, const glm::vec2& direction);
};

