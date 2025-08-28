#include "VehicleCollisionSystem.h"
#include <Engine/Scene/Scene.h>

#include <Engine/Debug/Instrumentor.h>
#include <Engine/Events/Public/CollisionEvents.h>


void VehicleCollisionSystem::UpdateVehicleCollision(float deltaTime, Engine::Scene* scene)
{
    EE_PROFILE_FUNCTION();

    // Snapshot GPU collision results for this frame
    const auto collisions = Engine::CollisionResultsCPU::LatestProjectiles;

    scene->ForEach<Engine::TransformComponent, VehicleComponent, Engine::IDComponent>(
        [&](Engine::Entity vehicleEntity, Engine::TransformComponent& vehicleTransformComp,
            VehicleComponent& vehicleComp, Engine::IDComponent& vehicleIDComp)
        {
            // Find the first collision that targets this vehicle
            for (const auto& col : collisions)
            {
                if (vehicleIDComp.ID != col.GetEntityID())
                    continue;

                const uint32_t objectHealth = col.Health;     // remaining HP at hit pixel
                const uint32_t kMaxHealth = 255;
                const float    healthRatio = (kMaxHealth > 0)
                    ? float(objectHealth) / float(kMaxHealth)
                    : 1.0f;

                // Tunables
                const float kPushbackStrength = 0.8f;
                const float kVelocityNudge = 0.3f;

                if (healthRatio < 0.10f)
                {
                    // Low health at contact => mostly let vehicle push through
                    const float slowFactor = glm::mix(0.9f, 0.5f, healthRatio);
                    vehicleComp.CurrentSpeed *= slowFactor;
                    // (Optional) tiny nudge/push if you still want feedback:
                    // ApplyPush(vehicleTransformComp, vehicleComp, col.HitPosition,
                    //           kPushbackStrength * 0.3f, kVelocityNudge * 0.5f);
                }
                else
                {
                    // High health => bounce back and slow down more
                    ApplyPush(vehicleTransformComp, vehicleComp, col.HitPosition,
                        kPushbackStrength, kVelocityNudge);
                    vehicleComp.CurrentSpeed *= 0.3f;
                }

                // Handle only one contact per vehicle per tick (tweak if needed)
                break;
            }
        });
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


