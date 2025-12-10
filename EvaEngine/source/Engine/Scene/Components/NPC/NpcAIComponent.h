#pragma once
#include <vector>
#include <glm/glm.hpp>
#include <cstdint>

enum class AIState
{
    Idle,
    Patrol,
    MoveToTarget,
    ChaseLOS,
    Attack
};

struct NPCAIMovementComponent
{
    AIState CurrentState = AIState::Idle;

    // existing:
    float IdleTimer = 0.0f;
    float IdleDuration = 1.0f;
    std::vector<glm::vec3> PatrolPoints;
    size_t CurrentPatrolIndex = 0;
    glm::vec3 LastKnownTargetPos{ 0.0f };
    bool      HasLastKnownTarget = false;
    glm::vec3 TargetPosition;
    float MoveSpeed = 3.0f;

    // NEW:
    std::vector<glm::vec3> Path;
    size_t PathIndex = 0;

    bool HasPath() const {
        return PathIndex < Path.size();
    }

    void ClearPath() {
        Path.clear();
        PathIndex = 0;
    }
};




struct NPCAIVisionComponent
{
    float ViewRadius = 100.0f;    
    float ViewAngle = 180.0f;    
    bool HasLineOfSight = false;

    // Internally set target
    entt::entity VisibleTarget = entt::null;
};
