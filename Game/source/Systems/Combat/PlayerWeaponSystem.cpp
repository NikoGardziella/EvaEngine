#include "PlayerWeaponSystem.h"
#include <Engine/Scene/Components/Combat/WeaponComponent.h>
#include <Engine/AssetManager/AssetManager.h>
#include <Engine/Debug/Instrumentor.h>
#include <Engine/Scene/Components/Vehicles/DriverComponent.h>
#include <Engine/Scene/Components/Projectiles/ProjectileComponent.h>
#include <Engine/Scene/Scene.h>


void PlayerWeaponSystem::UpdatePlayerWeaponSystem(float deltaTime,  Engine::Scene* scene)
{
    EE_PROFILE_FUNCTION();

    // Mouse world position from the primary camera
    glm::vec2 mouseWorldPosition{ 0.0f, 0.0f };
    bool hasPrimaryCamera = false;

    scene->ForEach<Engine::TransformComponent, Engine::CameraComponent>(
        [&](Engine::Entity /*cameraEntity*/,
            Engine::TransformComponent& cameraTransformComp,
            Engine::CameraComponent& cameraComp)
        {
            if (hasPrimaryCamera || !cameraComp.Primary)
            {
                return;
            }
            mouseWorldPosition = cameraComp.Camera.ScreenToWorld(cameraTransformComp.GetTransform());
            hasPrimaryCamera = true;
        });

    // Players with weapons
    scene->ForEach<Engine::TransformComponent, WeaponComponent>(
        [&](Engine::Entity playerEntity, Engine::TransformComponent& playerTransformComp, WeaponComponent& weaponComp)
        {
            // cooldown tick
            if (weaponComp.Cooldown > 0.0f)
                weaponComp.Cooldown -= deltaTime;

            if (!hasPrimaryCamera) return;

            // fire?
            if (Engine::Input::IsMouseButtonPressed(Engine::Mouse::Button0) &&
                weaponComp.Cooldown <= 0.0f)
            {
                const glm::vec2 playerPos = glm::vec2(playerTransformComp.Translation);
                glm::vec2 dir = mouseWorldPosition - playerPos;

                if (glm::length2(dir) > 1e-10f)
                {
                    dir = glm::normalize(dir);
                    ShootProjectile(playerEntity, playerTransformComp.Translation,
                        dir, scene, weaponComp);

                    weaponComp.Cooldown = weaponComp.FireRate;
                }
            }
        });
}


void PlayerWeaponSystem::ShootProjectile(Engine::Entity entity,
    const glm::vec2& position, const glm::vec2& direction, Engine::Scene* scene, const WeaponComponent& weaponComp)
{
    // disable shooting from a car now
    if (entity.HasComponent<DriverComponent>())
        return;

    Engine::Entity& projectileEntity = scene->CreateEntity("Projectile");

    Engine::TransformComponent& transformComp = projectileEntity.AddComponent<Engine::TransformComponent>();
	float lifeTime = 1.0f;
    float projectileRadius = 0.1f;
    ProjectileComponent& projectileComp = projectileEntity.AddComponent<ProjectileComponent>(direction * weaponComp.ProjectileSpeed, lifeTime);
    projectileComp.Damage = weaponComp.Damage;
    projectileComp.ProjectileRadius = projectileRadius;
    projectileComp.PixelDestructionRadius = weaponComp.DestructionRadius;

    Engine::SpriteRendererComponent& spriteComp = projectileEntity.AddComponent<Engine::SpriteRendererComponent>();
    projectileComp.Owner = entity;

    spriteComp.Texture = Engine::AssetManager::GetTexture("bullet");
    transformComp.Translation = glm::vec3(position, 0.0f);
    transformComp.Rotation.z = std::atan2(direction.y, direction.x);
}
