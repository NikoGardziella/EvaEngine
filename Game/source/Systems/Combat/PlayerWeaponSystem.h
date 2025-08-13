#pragma once
#include "entt.hpp"
#include "Engine.h"
#include <Engine/Scene/Components/Combat/WeaponComponent.h>


class PlayerWeaponSystem
{
public:
	static void UpdatePlayerWeaponSystem(entt::registry& registry, float deltaTime, Engine::Scene* scene);
	static void ShootProjectile(entt::registry& registry, Engine::Entity entity, const glm::vec2& position, const glm::vec2& direction, Engine::Scene* scene, const WeaponComponent& weaponComp);
};

