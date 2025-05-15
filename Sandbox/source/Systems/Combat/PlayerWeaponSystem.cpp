#include "PlayerWeaponSystem.h"
#include <Engine/Scene/Components/Player/CharacterControllerComponent.h>
#include <Engine/Scene/Components/Combat/WeaponComponent.h>
#include <Engine/AssetManager/AssetManager.h>
#include <Engine/Debug/Instrumentor.h>


void PlayerWeaponSystem::UpdatePlayerWeaponSystem(entt::registry& registry, float deltaTime, Engine::Scene* scene)
{
    EE_PROFILE_FUNCTION();

    auto view = registry.view<Engine::TransformComponent, CharacterControllerComponent, WeaponComponent>();

    glm::vec2 mouseScreen = Engine::Input::GetMouseScreenPosition();
    glm::vec2 mouseWorldPosition = glm::vec2(0.0f, 0.0f);
    {

        {
            auto group = registry.group<Engine::TransformComponent, Engine::CameraComponent>();
            for (auto entity : group)
            {
                auto [cameraTransformComp, cameraComp] = group.get<Engine::TransformComponent, Engine::CameraComponent>(entity);

                if (cameraComp.Primary)
                {
                    mouseWorldPosition = cameraComp.Camera.ScreenToWorld(mouseScreen, cameraTransformComp.GetTransform());
                    break;
                }
            }
        }
    }


    for (auto entity : view)
    {
        auto& transform = view.get<Engine::TransformComponent>(entity);
        auto& weapon = view.get<WeaponComponent>(entity);

        if (weapon.Cooldown > 0.0f)
            weapon.Cooldown -= deltaTime;

        if (Engine::Input::IsMouseButtonPressed(Engine::Mouse::Button0) && weapon.Cooldown <= 0.0f)
        {
            glm::vec2 playerPos = glm::vec2(transform.Translation);
            glm::vec2 direction = glm::normalize(mouseWorldPosition - playerPos);

            ShootProjectile(registry, entity, transform.Translation, direction, scene, weapon.Damage);

            weapon.Cooldown = weapon.FireRate;
        }
    }
}

void PlayerWeaponSystem::ShootProjectile(entt::registry& registry, entt::entity entity, const glm::vec2& position, const glm::vec2& direction, Engine::Scene* scene, float damage)
{
    Engine::Entity& projectileEntity = scene->CreateEntity("Projectile");

    Engine::TransformComponent& transformComp = projectileEntity.AddComponent<Engine::TransformComponent>();
    Engine::ProjectileComponent& projectileComp = projectileEntity.AddComponent<Engine::ProjectileComponent>(direction, 5.0f);
    projectileComp.Damage = damage;
    Engine::SpriteRendererComponent& spriteComp = projectileEntity.AddComponent<Engine::SpriteRendererComponent>();

    projectileComp.Owner = entity;

    spriteComp.Texture = Engine::AssetManager::GetTexture("bullet");
    transformComp.Translation = glm::vec3(position, 0.0f);
    transformComp.Rotation.z = std::atan2(direction.y, direction.x);
}
