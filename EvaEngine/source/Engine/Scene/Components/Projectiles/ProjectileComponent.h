#pragma once
#include <glm/ext/vector_float2.hpp>
#include <Engine/Scene/Entity.h>


struct ProjectileComponent
{
    glm::vec2  Direction;   // units per second
    glm::vec2  TargetPositionAtFireTime;   // where mouse was on shoot
    float      DistanceToTargetatFireTime;
    float      ProjectileMaxRange;   // range
    uint32_t   Damage = 1;
    float      ProjectileRadius = 0.1f;
    float      TargetPositionHeightZ1;
    float      ProjectileSped;
    float      DestructionRadius = 0.1f; 
    Engine::Entity   Owner;

    ProjectileComponent() = default;
    ProjectileComponent(const glm::vec2& direction, float projectileMaxRange)
        : Direction(direction), ProjectileMaxRange(projectileMaxRange) {
    }
};
