#include "PlayerMovementSystem.h"
#include <Engine/Scene/Components/Player/CharacterControllerComponent.h>

void PlayerMovementSystem::MovementSystem(entt::registry& registry, float deltaTime)
{
    auto view = registry.view<Engine::TransformComponent, CharacterControllerComponent>();

    for (auto entity : view)
    {
        Engine::TransformComponent& transform = view.get<Engine::TransformComponent>(entity);
        CharacterControllerComponent& controllerComp = view.get<CharacterControllerComponent>(entity);

        // Skip if velocity is zero (e.g., stopped due to collision)
        if (glm::length2(controllerComp.velocity) < 0.0001f)
            continue;

        // Apply movement
       // transform.Translation.x += controllerComp.velocity.x * deltaTime * controllerComp.speed;
        //transform.Translation.y += controllerComp.velocity.y * deltaTime * controllerComp.speed;
    }
}
