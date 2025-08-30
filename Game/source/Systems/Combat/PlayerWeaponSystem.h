#pragma once
#include "Engine.h"
#include <Engine/Scene/Components/Combat/WeaponComponent.h>
#include <Engine/Scene/Scene.h>


class Scene;
class PlayerWeaponSystem
{
public:
	static void UpdatePlayerWeaponSystem(float deltaTime, Engine::Scene* scene);
	
private:
	static void ShootProjectile(Engine::Entity entity, const glm::vec2& position, const glm::vec2& mouseWorldPosition, Engine::Scene* scene, const WeaponComponent& weaponComp);
	static float SampleHeightAt(Engine::Scene* scene, const glm::vec2& worldXY, int radiusPx = 0);
};

