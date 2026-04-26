#pragma once
#include <Engine/Scene/Components/Combat/WeaponComponent.h>
#include <Engine/Scene/Scene.h>
#include <Engine/Scene/Component.h>


class Scene;
class PlayerWeaponSystem
{
public:
	static void UpdatePlayerWeaponSystem(float deltaTime, Engine::Scene* scene);

	static Engine::Entity SpawnProjectileEntity(Engine::Scene* scene, Engine::Entity owner, const glm::vec2& origin, const glm::vec2& direction, const WeaponComponent& , const glm::vec2& mouseWorld);

	static void FireShotgunWeapon(Engine::Entity player, Engine::TransformComponent& tr, const glm::vec2& mouseWorld, const WeaponComponent& weapon, Engine::Scene* scene);

	static void FireExplosiveProjectileWeapon(Engine::Entity player, Engine::TransformComponent& tr, const glm::vec2& mouseWorld, const WeaponComponent& weapon, Engine::Scene* scene);

	static void FireMeleeWeapon(Engine::Entity player, Engine::TransformComponent& tr, const WeaponComponent& weapon, Engine::Scene* scene);
	
private:
	static void FireSingleProjectileWeapon(Engine::Entity player, Engine::TransformComponent& tr, const glm::vec2& mouseWorld, const WeaponComponent& weapon, Engine::Scene* scene);
	static void FireThrowableWeapon(Engine::Entity player, Engine::TransformComponent& transformComp, const glm::vec2& mouseWorld, const WeaponComponent& weaponComp, Engine::Scene* scene);
	static std::vector<uint64_t> BuildProjectileAffectedUIDs(Engine::Scene* scene, const glm::vec2& origin, const glm::vec2& target, float projectileRadius, float destructionRadius);
	static float SampleHeightAt(Engine::Scene* scene, const glm::vec2& worldXY, int radiusPx = 0);
};

