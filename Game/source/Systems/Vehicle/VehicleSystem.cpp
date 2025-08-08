#include "VehicleSystem.h"
#include <Engine/Scene/Scene.h>
#include <Engine/Scene/Components/Vehicles/VehicleComponent.h>
#include <Engine/Debug/Instrumentor.h>

void VehicleSystem::UpdateVehicleSystem(entt::registry& registry, float deltaTime, Engine::Scene* scene)
{
    EE_PROFILE_FUNCTION();

    auto view = registry.view<VehicleComponent, Engine::TransformComponent, Engine::IDComponent>();
    // Inside your VehicleSystem update loop
    for (auto entity : view)
    {
        auto& vehicle = view.get<VehicleComponent>(entity);
        auto& transform = view.get<Engine::TransformComponent>(entity);
        auto& IDComp = view.get<Engine::IDComponent>(entity);

        const float linearFriction = 0.90f;
        const float angularFriction = 0.90f;

        // === Speed Calculation ===
        if (glm::length(vehicle.Velocity) > 0.0f)
        {
            float engineForce = vehicle.Power * glm::sign(vehicle.Velocity.y);
            float acceleration = engineForce / vehicle.Mass;
            vehicle.CurrentSpeed += acceleration * deltaTime;
        }
        else
        {
            // Decelerate when no input
            float deceleration = vehicle.Deceleration / vehicle.Mass * 100.0f; // arbitrary multiplier
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

        // Clamp speed
        vehicle.CurrentSpeed = glm::clamp(vehicle.CurrentSpeed, -vehicle.MaxSpeed, vehicle.MaxSpeed);

        // === Pushback ===
        if (glm::length(vehicle.Pushback) > 0.001f)
        {
            transform.Translation += glm::vec3(vehicle.Pushback * deltaTime, 0.0f);

            // Decay pushback over time
            float pushDecay = 5.0f; // tweak as needed
            vehicle.Pushback = glm::mix(vehicle.Pushback, glm::vec2(0.0f), pushDecay * deltaTime);
        }
        else
        {
            vehicle.Pushback = glm::vec2(0.0f);
        }

        // === Movement and Steering ===
        float rotationRadians = transform.Rotation.z;
        glm::vec2 forward = glm::vec2(glm::cos(rotationRadians), glm::sin(rotationRadians));
        glm::vec2 movement = forward * vehicle.CurrentSpeed * deltaTime;

        transform.Translation += glm::vec3(movement, 0.0f);

        if (vehicle.CurrentSpeed > 0.9f || vehicle.CurrentSpeed < -0.9f)
        {
            float steering = -vehicle.Velocity.x * 2.0f; // arbitrary factor
            transform.Rotation.z += steering * deltaTime;
        }

        // === Friction ===
        vehicle.Velocity.y *= linearFriction;
        vehicle.Velocity.x *= angularFriction;
    }





}

