#pragma once
#include <glm/ext/vector_float2.hpp>
#include <Engine/Scene/Entity.h>


struct ProjectileComponent
{
    glm::vec2 Velocity;   // units per second
    float      LifeTime;   // range
    float      Damage = 10.0f;
    float      Radius = 1.0f;
    Engine::Entity   Owner;

    ProjectileComponent() = default;
    ProjectileComponent(const glm::vec2& velocity, float lifeTime)
        : Velocity(velocity), LifeTime(lifeTime) {
    }
};
