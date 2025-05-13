#pragma once
#include <vector>
#include <glm/glm.hpp>
#include <cstdint>

enum class AIState
{
    Idle,
    Patrol,
    MoveToTarget
};

struct NPCAIMovementComponent
{
    AIState CurrentState = AIState::Idle;

    // For patrol
    std::vector<glm::vec3> PatrolPoints;
    int CurrentPatrolIndex = 0;

    // For idle
    float IdleDuration = 2.0f;
    float IdleTimer = 0.0f;

    // For MoveToTarget
    glm::vec3 TargetPosition;

    float MoveSpeed = 2.0f;
};



struct NPCAIVisionComponent
{
    float ViewRadius = 10.0f;    
    float ViewAngle = 360.0f;    
    bool HasLineOfSight = false;

    // Internally set target
    entt::entity VisibleTarget = entt::null;
};
