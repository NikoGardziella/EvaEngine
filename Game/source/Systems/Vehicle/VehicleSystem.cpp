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

        bool collision = false;
        for (auto& col : Engine::CollisionResultsCPU::LatestProjectiles)
        {
            if (IDComp.ID == col.GetProjectileID())
            {
                collision = true;
                EE_INFO("vehicle collided");
                break;
            }
        }

        if (!collision)
        {
            if (glm::length(vehicle.Velocity) > 0.0f)
            {
                float engineForce = vehicle.Power * glm::sign(vehicle.Velocity.y);
                float acceleration = engineForce / vehicle.Mass;
                vehicle.CurrentSpeed += acceleration * deltaTime;
            }
            else
            {
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

            vehicle.CurrentSpeed = glm::clamp(vehicle.CurrentSpeed, -vehicle.MaxSpeed, vehicle.MaxSpeed);
        }
        else
        {
            // Collision: stop or reverse slightly
            vehicle.CurrentSpeed = -0.3f * glm::abs(vehicle.CurrentSpeed); // Pushback
        }

        float rotationRadians = transform.Rotation.z;
        glm::vec2 forward = glm::vec2(glm::cos(rotationRadians), glm::sin(rotationRadians));
        glm::vec2 movement = forward * vehicle.CurrentSpeed * deltaTime;

        transform.Translation += glm::vec3(movement, 0.0f);

        if (vehicle.CurrentSpeed != 0.0f)
        {
            float steering = -vehicle.Velocity.x * 3.0f;
            transform.Rotation.z += steering * deltaTime;
        }

        vehicle.Velocity.y *= linearFriction;
        vehicle.Velocity.x *= angularFriction;
    }




}

