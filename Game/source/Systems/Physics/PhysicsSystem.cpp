#include "PhysicsSystem.h"

#include <Engine/Debug/Instrumentor.h>
#include <Engine/Scene/Scene.h>
#include "Engine/Scene/Components/Physics/PhysicsComponent.h"

void PhysicsSystem::UpdatePhysicsSystem(float dt, Engine::Scene* scene)
{
    EE_PROFILE_FUNCTION();

    scene->ForEach<Engine::TransformComponent, PhysicsComponent>(
        [dt](Engine::Entity e, Engine::TransformComponent& xf,  PhysicsComponent& phys)
        {
            if (!phys.active || phys.timeLeft <= 0.0f) {
                if (phys.destroyOnFinish) 
                {
                    // optional: scene->DestroyEntity(e);
                }
                phys.active = false;
                return;
            }

            // Integrate (semi-implicit Euler)
            phys.velocity += phys.gravity * dt;

            // Exponential damping per second
            const float ld = glm::clamp(phys.linearDamping, 0.f, 0.999f);
            const float ad = glm::clamp(phys.angularDamping, 0.f, 0.999f);
            phys.velocity *= std::pow(1.f - ld, dt);
            phys.angularVelocity *= std::pow(1.f - ad, dt);

            // Apply to transform
            glm::vec3 p3 = xf.Translation;
            p3.x += phys.velocity.x * dt;
            p3.y += phys.velocity.y * dt;
            xf.Translation = p3;

            // If your transform supports rotation around Z:
            float newAngle = xf.Rotation.z + phys.angularVelocity * dt;
            xf.Rotation.z = newAngle;

            // Mirror to renderer’s bindless slot origins so textures move with the entity
            // (if your renderer uses m_slotOriginWorld; skip if it uses Transform directly)
          

            // Timebox
            phys.timeLeft -= dt;
            if (phys.timeLeft <= 0.0f) {
                phys.active = false;
                // Snap or leave as-is; your call. You can also zero out velocities:
                phys.velocity = {};
                phys.angularVelocity = 0.f;
            }
        });
}

