
#include "CharacterControllerSystem.h"
#include "Engine/Core/Input.h"
#include "Engine/Events/KeyCode.h"
#include <Engine/Debug/Instrumentor.h>

#include <glm/glm.hpp>
#include <Engine/Scene/Components/Player/CharacterControllerComponent.h>

#include "Engine/Scene/Entity.h"
#include <Engine/Scene/Components/Vehicles/VehicleComponent.h>
#include <Engine/Scene/Components/Vehicles/DriverComponent.h>

void CharacterControllerSystem::UpdateCharacterControllerSystem(entt::registry& registry, float deltaTime, Engine::Scene* scene)
{
    EE_PROFILE_FUNCTION();

   
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
                    mouseWorldPosition = cameraComp.Camera.ScreenToWorld(cameraTransformComp.GetTransform());
                    break;
                }
            }
        }
    }
    Engine::Entity playerEntity = Engine::Entity{};
    auto view = registry.view<Engine::TransformComponent, CharacterControllerComponent>();

    for (auto entity : view)
    {
        auto& playerTransformComp = view.get<Engine::TransformComponent>(entity);
        auto& controllerComp = view.get<CharacterControllerComponent>(entity);

        glm::vec2 diff = glm::vec2(mouseWorldPosition) - glm::vec2(playerTransformComp.Translation);

        if (glm::length(diff) > 0.0001f)
        {
            glm::vec2 direction = glm::normalize(diff);
            float angle = std::atan2(direction.y, direction.x);
            playerTransformComp.Rotation.z = angle + glm::radians(220.0f);
        }

        glm::vec2 inputVelocity = { 0.0f, 0.0f };
        if (Engine::Input::IsKeyPressed(Engine::Key::A)) inputVelocity.x -= 1.0f;
        if (Engine::Input::IsKeyPressed(Engine::Key::D)) inputVelocity.x += 1.0f;
        if (Engine::Input::IsKeyPressed(Engine::Key::W)) inputVelocity.y += 1.0f;
        if (Engine::Input::IsKeyPressed(Engine::Key::S)) inputVelocity.y -= 1.0f;

        playerEntity = Engine::Entity{ entity, scene };
        if (playerEntity.HasComponent<DriverComponent>())
        {
            auto& driverComp = playerEntity.GetComponent<DriverComponent>();
            if (driverComp.Vehicle.HasComponent<VehicleComponent>())
            {
                auto& vehicleComp = driverComp.Vehicle.GetComponent<VehicleComponent>();
                vehicleComp.Velocity = inputVelocity;
                controllerComp.velocity = glm::vec3(0);
                playerTransformComp.Translation = driverComp.Vehicle.GetComponent<Engine::TransformComponent>().Translation;
            }
        }
        else
        {
            controllerComp.velocity = inputVelocity;
        }
    }

    // Handle enter/exit once per frame
    if (Engine::Input::IsKeyPressed(Engine::Key::F))
    {
        float minDistanceToEnterVehicle = 2.0f;

        auto vehicleView = scene->GetAllEntitiesWith<Engine::TransformComponent, VehicleComponent>();
        for (auto vehicleEntity : vehicleView)
        {
            auto& vehicleTransform = vehicleView.get<Engine::TransformComponent>(vehicleEntity);
            auto& vehicleComp = vehicleView.get<VehicleComponent>(vehicleEntity);

            // Skip if cooldown is active
            if (vehicleComp.ExitEnterCooldown > 0.0f)
                continue;

            auto& playerTransform = playerEntity.GetComponent<Engine::TransformComponent>();
            float distance = glm::distance(vehicleTransform.Translation, playerTransform.Translation);

            if (distance < minDistanceToEnterVehicle)
            {
                if (playerEntity.HasComponent<DriverComponent>())
                {
                    // Exit vehicle
                    auto& driverComp = playerEntity.GetComponent<DriverComponent>();
                    auto& vehicle = driverComp.Vehicle;
                    if (vehicle.HasComponent<VehicleComponent>())
                    {
                        auto& targetVehicleComp = vehicle.GetComponent<VehicleComponent>();
                        targetVehicleComp.Driver = Engine::Entity{};
                        targetVehicleComp.ExitEnterCooldown = 1.0f;
                    }
                    playerEntity.RemoveComponent<DriverComponent>();
                }
                else if (!vehicleComp.Driver)
                {
                    // Enter vehicle
                    vehicleComp.Driver = playerEntity;
                    vehicleComp.ExitEnterCooldown = 1.0f;
                    playerEntity.AddComponent<DriverComponent>(Engine::Entity{ vehicleEntity, scene });
                }
                break;
            }
        }
    }

    {
        auto vehicleView = scene->GetAllEntitiesWith<VehicleComponent>();
        for (auto vehicleEntity : vehicleView)
        {
            auto& vehicleComp = vehicleView.get<VehicleComponent>(vehicleEntity);
            if (vehicleComp.ExitEnterCooldown > 0.0f)
                vehicleComp.ExitEnterCooldown -= deltaTime;
        }
    }

    

}

