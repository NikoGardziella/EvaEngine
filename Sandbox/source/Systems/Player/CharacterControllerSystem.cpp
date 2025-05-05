
#include "CharacterControllerSystem.h"
#include "Engine/Core/Input.h"
#include "Engine/Core/Core.h"
#include "Engine/Events/KeyCode.h"
#include "Engine/Events/MouseCodes.h"
#include "glm/glm.hpp"
#include <Engine/Debug/Instrumentor.h>

void CharacterControllerSystem::UpdateCharacterControllerSystem(entt::registry& registry, float deltaTime)
{
    EE_PROFILE_FUNCTION();

    auto view = registry.view<Engine::TransformComponent, Engine::CharacterControllerComponent>();

    glm::vec2 mouseScreen = Engine::Input::GetMousePosition(); // screen space
   // glm::vec3 mouseWorld = Engine::Camera::ScreenToWorld(mouseScreen); // world space

    for (auto entity : view)
    {
        auto& transform = view.get<Engine::TransformComponent>(entity);
        auto& controller = view.get<Engine::CharacterControllerComponent>(entity);

        glm::vec2 input = { 0.0f, 0.0f };

        if (Engine::Input::IsKeyPressed(Engine::Key::A)) input.x -= 1.0f;
        if (Engine::Input::IsKeyPressed(Engine::Key::D)) input.x += 1.0f;
        if (Engine::Input::IsKeyPressed(Engine::Key::W)) input.y += 1.0f;
        if (Engine::Input::IsKeyPressed(Engine::Key::S)) input.y -= 1.0f;
        if (Engine::Input::IsMouseButtonPressed(Engine::Mouse::Button0))
        {
			//EE_INFO("Mouse Button0 Pressed, spawn projectile");

        }

        controller.velocity = input * controller.speed;
        transform.Translation += glm::vec3(controller.velocity * deltaTime, 0.0f);

       // glm::vec2 direction = glm::normalize(glm::vec2(mouseWorld) - glm::vec2(transform.Translation));
        //float angle = std::atan2(direction.y, direction.x); // in radians

       // transform.Rotation = glm::quat(glm::vec3(0.0f, 0.0f, angle)); // 
    }
}