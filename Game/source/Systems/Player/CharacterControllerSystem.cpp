
#include "CharacterControllerSystem.h"
#include "Engine/Core/Input.h"
#include "Engine/Events/KeyCode.h"
#include <Engine/Debug/Instrumentor.h>

#include <glm/glm.hpp>
#include <Engine/Scene/Components/Player/CharacterControllerComponent.h>

#include "Engine/Scene/Entity.h"
#include <Engine/Scene/Components/Vehicles/VehicleComponent.h>
#include <Engine/Scene/Components/Vehicles/DriverComponent.h>
#include <Engine/Scene/Scene.h>
#include <Engine/Scene/Components/Render/3D/AnimatorComponent.h>

void CharacterControllerSystem::UpdateCharacterControllerSystem(float deltaTime, Engine::Scene* scene)
{
    EE_PROFILE_FUNCTION();

    // --- 1) Mouse world position from the primary camera ---
    glm::vec2 mouseWorldPosition{ 0.0f, 0.0f };

    scene->ForEach<Engine::TransformComponent, Engine::CameraComponent>(
        [&](Engine::Entity, Engine::TransformComponent& camXform, Engine::CameraComponent& cam)
        {
            if (cam.Primary)
            {
                mouseWorldPosition = cam.Camera.ScreenToWorld(camXform.GetTransform());
            }
        });


    Engine::Entity playerEntity;
    scene->ForEach<Engine::TransformComponent, CharacterControllerComponent>(
        [&](Engine::Entity entity,  Engine::TransformComponent& playerTransformComp,
            CharacterControllerComponent& controller)
        {

            if (!entity.HasComponent<CharacterControllerComponent>())
            {
                return;
            }
            playerEntity = entity;

            glm::vec2 diff = mouseWorldPosition - glm::vec2(playerTransformComp.Translation);
            if (glm::length(diff) > 0.0001f)
            {
                glm::vec2 dir = glm::normalize(diff);

                float angle2D = std::atan2(dir.y, dir.x);

                float anotherRandomOffsetThatMightBeMovedSOmwhereElse = 90.0f;
                angle2D += glm::radians(anotherRandomOffsetThatMightBeMovedSOmwhereElse);

                playerTransformComp.Rotation.z = angle2D; 

            }

            
            glm::vec2 inputVelocity{ 0.0f };
            if (Engine::Input::IsKeyPressed(Engine::Key::A)) inputVelocity.x -= 1.0f;
            if (Engine::Input::IsKeyPressed(Engine::Key::D)) inputVelocity.x += 1.0f;
            if (Engine::Input::IsKeyPressed(Engine::Key::W)) inputVelocity.y += 1.0f;
            if (Engine::Input::IsKeyPressed(Engine::Key::S)) inputVelocity.y -= 1.0f;

            
            if (playerEntity.HasComponent<Engine::Animator3DComponent>())
            {
                Engine::Animator3DComponent& playerAnimComp = playerEntity.GetComponent<Engine::Animator3DComponent>();

                if (inputVelocity.x != 0.0f || inputVelocity.y != 0.0f)
                {
                    playerAnimComp.clipA = 0; // run
                }
                else
                {
                    playerAnimComp.clipA = 1; // idle
                }


            }

            if (playerEntity.HasComponent<DriverComponent>()) 
            {
                auto& driver = playerEntity.GetComponent<DriverComponent>();
                if (driver.Vehicle && driver.Vehicle.HasComponent<VehicleComponent>())
                {
                    auto& vehicle = driver.Vehicle.GetComponent<VehicleComponent>();
                    vehicle.Velocity = inputVelocity;         // steer the vehicle
                    controller.velocity = glm::vec3(0.0f);    // player stands still while driving
                    // keep player “inside” vehicle transform:
                    playerTransformComp.Translation = driver.Vehicle.GetComponent<Engine::TransformComponent>().Translation;
                }
                else 
                {
                    controller.velocity = glm::vec3(inputVelocity, 0.0f);
                }
            }
            else
            {
                controller.velocity = glm::vec3(inputVelocity, 0.0f);
            }
        });

    // --- 3) Enter / Exit vehicle (press F) ---
    if (Engine::Input::IsKeyPressed(Engine::Key::F) && playerEntity)
    {
        constexpr float kEnterDistance = 2.0f;

        // Player transform once
        auto& playerXform = playerEntity.GetComponent<Engine::TransformComponent>();

        bool handled = false;
        scene->ForEach<Engine::TransformComponent, VehicleComponent>(
            [&](Engine::Entity vehEnt, Engine::TransformComponent& vehXform, VehicleComponent& veh)
            {
                if (handled) return;
                if (veh.ExitEnterCooldown > 0.0f) return;

                float d = glm::distance(vehXform.Translation, playerXform.Translation);
                if (d >= kEnterDistance) return;

                if (playerEntity.HasComponent<DriverComponent>())
                {
                    // Exit current vehicle
                    auto& driver = playerEntity.GetComponent<DriverComponent>();
                    if (driver.Vehicle && driver.Vehicle.HasComponent<VehicleComponent>())
                    {
                        auto& v = driver.Vehicle.GetComponent<VehicleComponent>();
                        v.Driver = Engine::Entity{};
                        v.ExitEnterCooldown = 1.0f;
                    }
                    playerEntity.RemoveComponent<DriverComponent>();
                }
                else if (!veh.Driver)
                {
                    // Enter this vehicle
                    veh.Driver = playerEntity;
                    veh.ExitEnterCooldown = 1.0f;
                    playerEntity.AddComponent<DriverComponent>(Engine::Entity{ vehEnt, scene });
                }
                handled = true;
            });
    }

    // --- 4) Cooldown tick ---
    scene->ForEach<VehicleComponent>(
        [&](Engine::Entity, VehicleComponent& v)
        {
            if (v.ExitEnterCooldown > 0.0f)
                v.ExitEnterCooldown -= deltaTime;
        });
}
