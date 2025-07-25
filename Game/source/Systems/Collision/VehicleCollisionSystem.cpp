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
            float pushbackStrength = 0.5f;
            ApplyPush(vehicleTransform, vehicle, collision.HitPosition, pushbackStrength);
            break;
        }
    }

    if (collided)
    {
        EE_INFO("vehicle collision");
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

        // Optional: dampen a bit to avoid ricocheting forever
        vehicle.CurrentSpeed *= 0.7f;
    }
}


