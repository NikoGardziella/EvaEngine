#include "VehicleCollisionSystem.h"
#include <Engine/Scene/Scene.h>

#include <Engine/Debug/Instrumentor.h>
#include <Engine/Events/Public/CollisionEvents.h>

void VehicleCollisionSystem::UpdateVehicleCollision(entt::registry& registry, float deltaTime, Engine::Scene* scene)
{
    EE_PROFILE_FUNCTION();

    auto vehicleView = registry.view<Engine::TransformComponent, VehicleComponent, Engine::IDComponent>();
    bool collided = false;

    for (auto vehicleEntity : vehicleView)
    {
        auto& vehicleTransform = vehicleView.get<Engine::TransformComponent>(vehicleEntity);
        auto& vehicle = vehicleView.get<VehicleComponent>(vehicleEntity);
        auto& IDComp = vehicleView.get<Engine::IDComponent>(vehicleEntity);

        for (const auto& collision : Engine::CollisionResultsCPU::LatestProjectiles)
        {
            if (IDComp.ID != collision.GetEntityID())
                continue;

            collided = true;

            // Query health of collided object (implement GetObjectHealthByID)
            uint32_t objectHealth = collision.Health;
            uint32_t maxHealth = 255;

            float healthRatio = (maxHealth > 0) ? (float)objectHealth / maxHealth : 1.0f;

            float pushbackStrength = 0.8f;
            float velocityNudge = 0.3f;

            if (healthRatio < 0.10f)
            {
                float slowFactor = glm::mix(0.9f, 0.5f, healthRatio); // mix(minSlow, maxSlow, ratio)

                // Low health: allow push through, small pushback, mild slow
                //ApplyPush(vehicleTransform, vehicle, collision.HitPosition, pushbackStrength * 0.3f, velocityNudge * 0.5f);
                vehicle.CurrentSpeed *= slowFactor;
                //EE_INFO("low health, healthRatio {}", healthRatio);
            }
            else
            {
                // High health: strong pushback, big slow
                ApplyPush(vehicleTransform, vehicle, collision.HitPosition, pushbackStrength, velocityNudge);
                vehicle.CurrentSpeed *= 0.3f;
                //EE_INFO("high health, healthRatio {}", healthRatio);
            }

            break;
        }

    }



}


void VehicleCollisionSystem::ApplyPush(
    Engine::TransformComponent& transform,
    VehicleComponent& vehicle,
    const glm::vec2& sourcePosition,
    float basePushStrength,
    float baseVelocityNudge
)
{
    glm::vec2 vehiclePos = glm::vec2(transform.Translation);
    glm::vec2 pushDir = glm::normalize(vehiclePos - sourcePosition);

    if (!glm::any(glm::isnan(pushDir)))
    {
      
        float scaledPushStrength = basePushStrength * vehicle.CurrentSpeed;
        float scaledVelocityNudge = baseVelocityNudge * vehicle.CurrentSpeed;

        vehicle.Pushback += pushDir * scaledPushStrength;
        vehicle.Velocity += pushDir * scaledVelocityNudge;

        vehicle.CurrentSpeed *= 0.7f;
    }
}


