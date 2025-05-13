
#include "CharacterControllerSystem.h"
#include "Engine/Core/Input.h"
#include "Engine/Core/Core.h"
#include "Engine/Events/KeyCode.h"
#include "Engine/Events/MouseCodes.h"
#include <Engine/Debug/Instrumentor.h>

#include <glm/glm.hpp>
#include <Engine/AssetManager/AssetManager.h>
#include <Engine/Scene/Components/Player/CharacterControllerComponent.h>



void CharacterControllerSystem::UpdateCharacterControllerSystem(entt::registry& registry, float deltaTime)
{
    EE_PROFILE_FUNCTION();


    glm::vec2 mouseScreen = Engine::Input::GetMouseScreenPosition();
    glm::vec2 mouseWorldPosition = glm::vec2(0.0f, 0.0f);
    {
        EE_PROFILE_SCOPE("Get Update Runtime Camera");

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


    auto view = registry.view<Engine::TransformComponent, CharacterControllerComponent>();

    for (auto entity : view)
    {
        auto& playerTransformComp = view.get<Engine::TransformComponent>(entity);
        auto& controllerComp = view.get<CharacterControllerComponent>(entity);

      
        // rotate player toward mouse pos
        glm::vec2 direction = glm::normalize(glm::vec2(mouseWorldPosition) - glm::vec2(playerTransformComp.Translation));
        float angle = std::atan2(direction.y, direction.x);
        playerTransformComp.Rotation.z = angle - 65.0f;


		controllerComp.lastFireTime += deltaTime;

        glm::vec2 input = { 0.0f, 0.0f };

        if (Engine::Input::IsKeyPressed(Engine::Key::A)) input.x -= 1.0f;
        if (Engine::Input::IsKeyPressed(Engine::Key::D)) input.x += 1.0f;
        if (Engine::Input::IsKeyPressed(Engine::Key::W)) input.y += 1.0f;
        if (Engine::Input::IsKeyPressed(Engine::Key::S)) input.y -= 1.0f;
        if (Engine::Input::IsMouseButtonPressed(Engine::Mouse::Button0))
        {
            if (controllerComp.lastFireTime > controllerComp.fireRate)
            {
                ShootProjectile(registry, entity, playerTransformComp.Translation, direction);
				controllerComp.lastFireTime = 0.0f;
            }

        }


        controllerComp.velocity = input * controllerComp.speed;
        playerTransformComp.Translation += glm::vec3(controllerComp.velocity * deltaTime, 0.0f);

    }
}

void CharacterControllerSystem::ShootProjectile(entt::registry& registry, entt::entity entity, const glm::vec2& position, const glm::vec2& direction)
{
	const auto& projectileEntity = registry.create();
	auto& transformComp = registry.emplace<Engine::TransformComponent>(projectileEntity);
	auto& projectileComp = registry.emplace<Engine::ProjectileComponent>(projectileEntity, direction, 5.0f);
	auto& spriteComp = registry.emplace<Engine::SpriteRendererComponent>(projectileEntity);
    spriteComp.Texture = Engine::AssetManager::GetTexture("bullet");
	transformComp.Translation = glm::vec3(position, 0.0f);
	transformComp.Rotation.z = std::atan2(direction.y, direction.x);
	// Add other components as needed
}
