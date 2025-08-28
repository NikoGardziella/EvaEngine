#pragma once
#include "Engine.h"
#include <Engine/Scene/Components/Combat/WeaponComponent.h>
#include <Engine/Scene/Scene.h>

class PlayerWeaponSystem
{
public:
	static void UpdatePlayerWeaponSystem(float deltaTime, Engine::Scene* scene);
	static void ShootProjectile(Engine::Entity entity, const glm::vec2& position, const glm::vec2& direction, Engine::Scene* scene, const WeaponComponent& weaponComp);
};

