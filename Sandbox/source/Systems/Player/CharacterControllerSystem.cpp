#include "pch.h"

#include "CharacterControllerSystem.h"
#include "Engine/Core/Input.h"
#include "Engine/Core/Core.h"
#include "Engine/Events/KeyCode.h"
#include "glm/glm.hpp"

void CharacterControllerSystem::UpdateCharacterControllerSystem(entt::registry& registry, float deltaTime)
{
    EE_PROFILE_FUNCTION();

    auto view = registry.view<Engine::TransformComponent, Engine::CharacterControllerComponent>();

    for (auto entity : view)
    {
        auto& transform = view.get<Engine::TransformComponent>(entity);
        auto& controller = view.get<Engine::CharacterControllerComponent>(entity);

        glm::vec2 input = { 0.0f, 0.0f };

        if (Engine::Input::IsKeyPressed(Engine::Key::A)) input.x -= 1.0f;
        if (Engine::Input::IsKeyPressed(Engine::Key::D)) input.x += 1.0f;
        if (Engine::Input::IsKeyPressed(Engine::Key::W)) input.y += 1.0f;
        if (Engine::Input::IsKeyPressed(Engine::Key::S)) input.y -= 1.0f;

        controller.velocity = input * controller.speed;
        transform.Translation += glm::vec3(controller.velocity * deltaTime, 0.0f);

    }
}