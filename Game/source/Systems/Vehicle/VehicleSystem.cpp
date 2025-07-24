#include "VehicleSystem.h"
#include <Engine/Scene/Scene.h>
#include <Engine/Scene/Components/Vehicles/VehicleComponent.h>
#include <Engine/Debug/Instrumentor.h>
void VehicleSystem::UpdateVehicleSystem(entt::registry& registry, float deltaTime, Engine::Scene* scene)
{
    EE_PROFILE_FUNCTION();

    auto view = registry.view<VehicleComponent, Engine::TransformComponent>();
    for (auto entity : view)
    {
        auto& vehicle = view.get<VehicleComponent>(entity);
        auto& transform = view.get<Engine::TransformComponent>(entity);

        const float linearFriction = 0.90f;
        const float angularFriction = 0.90f;

        // Acceleration force adjusted by mass: acceleration = force / mass
        // Here vehicle.Acceleration is considered max acceleration force
        if (vehicle.Velocity.y != 0.0f)
        {
            float engineForce = vehicle.Power * glm::sign(vehicle.Velocity.y);
            float acceleration = engineForce / vehicle.Mass;                 
            vehicle.CurrentSpeed += acceleration * deltaTime;
        }
        else
        {
            // Decelerate to stop if no input (friction adjusted by mass)
            float deceleration = vehicle.Deceleration / vehicle.Mass;
            if (vehicle.CurrentSpeed > 0.0f)
            {
                vehicle.CurrentSpeed -= deceleration * deltaTime;
                if (vehicle.CurrentSpeed < 0.0f)
                    vehicle.CurrentSpeed = 0.0f;
            }
            else if (vehicle.CurrentSpeed < 0.0f)
            {
                vehicle.CurrentSpeed += deceleration * deltaTime;
                if (vehicle.CurrentSpeed > 0.0f)
                    vehicle.CurrentSpeed = 0.0f;
            }
        }

        // Clamp to max speed
        vehicle.CurrentSpeed = glm::clamp(vehicle.CurrentSpeed, -vehicle.MaxSpeed, vehicle.MaxSpeed);

        // Movement
        float rotationRadians = transform.Rotation.z;
        glm::vec2 forward = glm::vec2(glm::cos(rotationRadians), glm::sin(rotationRadians));
        glm::vec2 movement = forward * vehicle.CurrentSpeed * deltaTime;
        transform.Translation += glm::vec3(movement, 0.0f);

        // Steering only if moving
        if (vehicle.CurrentSpeed != 0.0f)
        {
            float steering = -vehicle.Velocity.x * 3.0f;  // scale steering sensitivity
            transform.Rotation.z += steering * deltaTime;
        }

        // Apply friction to velocity input (this smoothes input changes)
        vehicle.Velocity.y *= linearFriction;
        vehicle.Velocity.x *= angularFriction;
    }




}

