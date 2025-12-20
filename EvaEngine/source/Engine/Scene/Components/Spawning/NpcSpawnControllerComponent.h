#pragma once
#include <vector>
#include <glm/glm.hpp>
#include <Engine.h>

namespace Engine {

    struct NpcSpawnControllerComponent
    {
        // ---- Limits / timing ----
        uint32_t maxAlive = 30;
        float spawnInterval = 1.0f;     // seconds between spawns
        float spawnTimer = 0.0f;        // runtime

        // ---- Spawn behavior ----
        uint32_t spawnBatch = 1;        // spawn N at a time when timer fires
        float spawnRadiusMin = 6.0f;
        float spawnRadiusMax = 10.0f;

        // Optional: if you want a "wave" style
        uint32_t totalToSpawn = 0;      // 0 = infinite
        uint32_t spawnedSoFar = 0;

        // ---- Enable/disable ----
        uint8_t enabled = 1;

        // ---- Debug / stats ----
        uint32_t aliveCached = 0;

        // ---- Optional tracking of spawned entities ----
        // If you track these, you can count alive without scanning all NPCs.
        // You must prune dead ones occasionally.
        std::vector<Entity> spawned;
        float pruneTimer = 0.0f;
        float pruneInterval = 1.0f;

        // ---- What to spawn (IDs into your registries / prefab IDs / etc.) ----
        uint32_t npcPrefabId = 0; // you define what this means
    };
}
