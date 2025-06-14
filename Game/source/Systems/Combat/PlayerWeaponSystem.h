#pragma once
#include "entt.hpp"
#include "Engine.h"


class PlayerWeaponSystem
{
public:
	static void UpdatePlayerWeaponSystem(entt::registry& registry, float deltaTime, Engine::Scene* scene);
	static void ShootProjectile(entt::registry& registry, entt::entity entity, const glm::vec2& position, const glm::vec2& direction, Engine::Scene* scene, float damage);
};

