#include "VehicleSystem.h"
#include <Engine/Scene/Scene.h>
#include <Engine/Scene/Components/Vehicles/VehicleComponent.h>
#include <Engine/Debug/Instrumentor.h>
#include <Engine/Scene/Entity.h>

void VehicleSystem::UpdateVehicleSystem(float deltaTime, Engine::Scene* scene)
{
    EE_PROFILE_FUNCTION();

    // Tunables
    constexpr float kLinearFriction = 0.90f;
    constexpr float kAngularFriction = 0.90f;
    constexpr float kSteerFactor = 2.0f;
    constexpr float kSteerSpeedGate = 0.5f;   // only steer when |speed| > gate
    constexpr float kPushDecayRate = 5.0f;   // larger = faster decay
    constexpr float kBrakeForce = 5.0f;

    scene->ForEach<VehicleComponent, Engine::TransformComponent, Engine::IDComponent>(
        [&](Engine::Entity /*entity*/, VehicleComponent& vehicleComp,
            Engine::TransformComponent& transformComp, Engine::IDComponent& /*idComp*/)
        {
            // --- Speed / engine ---
            if (glm::length(vehicleComp.Velocity) > 0.0f)
            {
                const float signY = glm::sign(vehicleComp.Velocity.y);
                const float invMass = (vehicleComp.Mass > 0.0f) ? (1.0f / vehicleComp.Mass) : 0.0f;

                // Check if input is opposite to current movement = braking
                const bool isBraking = (signY < 0.0f && vehicleComp.CurrentSpeed > 0.0f) ||
                    (signY > 0.0f && vehicleComp.CurrentSpeed < 0.0f);

                if (isBraking)
                {
                    // Apply strong braking force
                    const float brakeSign = (vehicleComp.CurrentSpeed > 0.0f) ? -1.0f : 1.0f;
                    vehicleComp.CurrentSpeed += brakeSign * vehicleComp.Power * kBrakeForce * invMass * deltaTime;

                    // Clamp to zero so we don't overshoot into reverse
                    if ((brakeSign < 0.0f && vehicleComp.CurrentSpeed < 0.0f) ||
                        (brakeSign > 0.0f && vehicleComp.CurrentSpeed > 0.0f))
                        vehicleComp.CurrentSpeed = 0.0f;
                }
                else
                {
                    // Normal acceleration
                    const float engineForce = vehicleComp.Power * signY;
                    vehicleComp.CurrentSpeed += (engineForce * invMass) * deltaTime;
                }
            }

            // Clamp max speed
            vehicleComp.CurrentSpeed = glm::clamp(vehicleComp.CurrentSpeed,
                -vehicleComp.MaxSpeed * 0.5f,  // reverse is slower
                vehicleComp.MaxSpeed);

            // --- External pushback (e.g., from collisions) ---
            if (glm::length2(vehicleComp.Pushback) > 1e-6f)
            {
                transformComp.Translation += glm::vec3(vehicleComp.Pushback * deltaTime, 0.0f);

                // Exponential-like decay (stable in [0,1])
                const float t = glm::clamp(kPushDecayRate * deltaTime, 0.0f, 1.0f);
                vehicleComp.Pushback = glm::mix(vehicleComp.Pushback, glm::vec2(0.0f), t);
            }
            else
            {
                vehicleComp.Pushback = glm::vec2(0.0f);
            }

            // --- Integrate movement in facing direction ---
            constexpr float kModelForwardOffset = glm::radians(270.f); // 90 degrees offset

            const float rot = transformComp.Rotation.z + kModelForwardOffset;
            const glm::vec2 forwardDir(std::cos(rot), std::sin(rot));
            const glm::vec2 deltaPos = forwardDir * vehicleComp.CurrentSpeed * deltaTime;
            transformComp.Translation += glm::vec3(deltaPos, 0.0f);

            // --- Steering (X is left/right input); only when moving enough ---
            if (std::abs(vehicleComp.CurrentSpeed) > kSteerSpeedGate)
            {
                const float steerDir = (vehicleComp.CurrentSpeed >= 0.0f) ? 1.0f : -1.0f;
                const float steerInput = -vehicleComp.Velocity.x * steerDir;
                transformComp.Rotation.z += (steerInput * kSteerFactor) * deltaTime;


            }

            // --- Input damping (friction-like) ---
            vehicleComp.Velocity.y *= kLinearFriction;   // throttle/brake fade
            vehicleComp.Velocity.x *= kAngularFriction;  // steering fade
        });
}

