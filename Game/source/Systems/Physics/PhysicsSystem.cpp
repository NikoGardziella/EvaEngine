#include "PhysicsSystem.h"

#include <Engine/Debug/Instrumentor.h>
#include <Engine/Scene/Scene.h>
#include <Engine/Scene/Entity.h>
#include "Engine/Scene/Components/Physics/PhysicsComponent.h"
#include <random>
#include <Engine/Scene/Component.h>

void PhysicsSystem::UpdatePhysicsSystem(float dt, Engine::Scene* scene)
{
    EE_PROFILE_FUNCTION();

    scene->ForEach<Engine::TransformComponent, PhysicsComponent>(
        [dt](Engine::Entity e, Engine::TransformComponent& xf, PhysicsComponent& phys)
        {
            if (!phys.active || phys.timeLeft <= 0.0f) 
            {
                if (phys.destroyOnFinish)
                {
                     
                }
                if (phys.removeOnFinish)
                {
                    e.RemoveComponent<PhysicsComponent>();
                    return;
                }
                phys.active = false;
                return;
            }

            // One-time random spin
            if (!phys.randomizedSpin)
            {
                phys.angularVelocity += RandomFloat(-6.0f, 6.0f);
                phys.randomizedSpin = true;
            }

            phys.velocity += phys.gravity * dt;

            const float ld = glm::clamp(phys.linearDamping, 0.f, 0.999f);
            const float ad = glm::clamp(phys.angularDamping, 0.f, 0.999f);
            phys.velocity *= std::pow(1.f - ld, dt);
            phys.angularVelocity *= std::pow(1.f - ad, dt);

            glm::vec3 p3 = xf.Translation;
            p3.x += phys.velocity.x * dt;
            p3.y += phys.velocity.y * dt;
            xf.Translation = p3;

            xf.Rotation.z += phys.angularVelocity * dt;

            phys.timeLeft -= dt;
            if (phys.timeLeft <= 0.0f) {
                phys.active = false;
                phys.velocity = {};
                phys.angularVelocity = 0.f;
            }
        });

}

inline float PhysicsSystem::RandomFloat(float minVal, float maxVal)
{
    static std::mt19937 rng{ std::random_device{}() };
    std::uniform_real_distribution<float> dist(minVal, maxVal);
    return dist(rng);
}