#pragma once
namespace Engine {

    struct NpcPrefab
    {
        const char* name = "Zombie";
        const char* meshKey = "zombieMesh1";
        uint32_t overrideSkeletonId = 0xFFFFFFFFu;

        // Spawn tuning
        float yawOffsetDeg = 0.0f;
        float pitchOffsetDeg = 180.0f;
        float viewAngleDeg = 360.0f;

        // AI tuning
        float moveSpeed = 3.0f;
        float radius = 0.30f;

        
        bool addPatrol = true;
    };

}
