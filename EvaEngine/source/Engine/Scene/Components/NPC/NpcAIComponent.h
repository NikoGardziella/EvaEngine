#pragma once
#include <vector>
#include <glm/glm.hpp>
#include <cstdint>
#include <Engine/Scene/Component.h>
#include "Engine/Scene/Entity.h"
#include "entt.hpp"

struct AgentRef
{
    Engine::TransformComponent* tr;
    float radius;
};




struct NpcAIPatrolComponent
{
    std::vector<glm::vec3> points;
    uint32_t index = 0;
};


struct NPCAIMovementComponent
{
    // ---- Local avoidance ----
    float radius = 0.30f;

    float moveSpeed = 3.0f;

    uint8_t  wantsMove = 0;              // 0/1
    uint8_t  usePath = 0;              // 0: direct seek, 1: pathfind/follow
    glm::vec2 goal2D = { 0.0f, 0.0f };  // destination in XY

    uint8_t  hasFacing = 0;
    glm::vec2 facing2D = { 1.0f, 0.0f };

    float   repathTimer = 0.0f;
    float   repathInterval = 0.35f;
    glm::vec2 lastRepathStart2D = { 0.0f, 0.0f };
    glm::vec2 lastRepathGoal2D = { 0.0f, 0.0f };

    std::vector<glm::vec3> path;
    uint32_t pathIndex = 0;

    bool HasPath() const
    {
        return pathIndex < (uint32_t)path.size();
    }

    void ClearPath()
    {
        path.clear();
        pathIndex = 0;
    }
};




struct NPCAIVisionComponent
{
    float ViewRadius = 30.0f;
    float ViewAngle = 360.0f;

    // Cached result (what other systems read)
    Engine::Entity VisibleTarget;
    bool hasLOS = false;
    float distToTarget = 0.0f;
    glm::vec3 lastSeenPos = glm::vec3(0.0f);
    float timeSinceSeen = 9999.0f;

    // Throttling
    float losCheckTimer = 0.0f;
    float losCheckInterval = 0.10f; // tweak (can be distance-based)

    // Optional: keep last known player entity even if momentarily not visible
    Engine::Entity lastSeenTarget;



    //debug 
    bool debugDraw = false;
};

