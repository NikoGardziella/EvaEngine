#include "NpcSpawnControllerSystem.h"
#include <Engine/Scene/Entity.h>
#include <Engine/Debug/Instrumentor.h>
#include <Engine/Scene/Components/Player/CharacterControllerComponent.h>
#include <Engine/Scene/Components/Spawning/NpcSpawnControllerComponent.h>
#include <Engine/Scene/Components/NPC/NpcAIComponent.h>
#include <Engine/Scene/SceneUtils/SpawnUtils.h>



void NpcSpawnControllerSystem::UpdateNpcSpawnControllerSystem(float dt, Engine::Scene* scene)
{
    EE_PROFILE_FUNCTION();

    // Find a spawn anchor (player). If you want, replace with a "SpawnAnchorComponent".
    glm::vec3 anchorPos(0.0f);
    bool hasAnchor = false;

    scene->ForEach<CharacterControllerComponent, Engine::TransformComponent>(
        [&](Engine::Entity, CharacterControllerComponent&, Engine::TransformComponent& tr)
        {
            if (hasAnchor) return;
            anchorPos = tr.Translation;
            hasAnchor = true;
        });

    if (!hasAnchor)
        return;

    // Count alive NPCs helper (scan). 
    auto CountAliveNPCs = [&]() -> uint32_t
        {
            uint32_t alive = 0;
            scene->ForEach<NPCAIMovementComponent>(
                [&](Engine::Entity, NPCAIMovementComponent&)
                {
                    alive++;
                });
            return alive;
        };

    scene->ForEach<Engine::NpcSpawnControllerComponent>(
        [&](Engine::Entity spawnerEnt, Engine::NpcSpawnControllerComponent& sp)
        {
            if (!sp.enabled)
                return;

            // Stop if wave complete
            if (sp.totalToSpawn != 0 && sp.spawnedSoFar >= sp.totalToSpawn)
                return;

            // Option A: use cached tracked entities
            // Option B: scan alive NPCs
            uint32_t alive = 0;

            if (!sp.spawned.empty())
            {
                // prune occasionally
                sp.pruneTimer += dt;
                if (sp.pruneTimer >= sp.pruneInterval)
                {
                    sp.pruneTimer = 0.0f;

                    auto& reg = scene->GetRegistry();
                    size_t w = 0;
                    for (size_t r = 0; r < sp.spawned.size(); ++r)
                    {
                        Engine::Entity ent = sp.spawned[r];
                        if (reg.valid(ent))
                        {
                            sp.spawned[w++] = ent;
                        }
                    }
                    sp.spawned.resize(w);
                }

                alive = (uint32_t)sp.spawned.size();
            }
            else
            {
                alive = CountAliveNPCs();
            }

            sp.aliveCached = alive;

            // Respect cap
            if (alive >= sp.maxAlive)
            {
                // keep timer from exploding (optional)
                sp.spawnTimer = std::min(sp.spawnTimer, sp.spawnInterval);
                return;
            }

            // Timer
            sp.spawnTimer += dt;
            if (sp.spawnTimer < sp.spawnInterval)
                return;

            sp.spawnTimer = 0.0f;

            // How many can we spawn this tick
            uint32_t canSpawn = sp.maxAlive - alive;
            uint32_t wantSpawn = sp.spawnBatch;
            uint32_t n = (wantSpawn < canSpawn) ? wantSpawn : canSpawn;

            if (sp.totalToSpawn != 0)
            {
                uint32_t remainingWave = sp.totalToSpawn - sp.spawnedSoFar;
                if (n > remainingWave) n = remainingWave;
            }

            if (n == 0)
                return;

            // Deterministic seed per spawner entity (so no system members)
            // If you prefer, add "uint32_t rngState" to component and use that.
            uint32_t seed = (uint32_t)(uintptr_t)spawnerEnt.GetUUID();

            for (uint32_t i = 0; i < n; ++i)
            {
                glm::vec2 dir = Engine::SpawnUtils::RandomUnit2D(seed);
                float r = Engine::SpawnUtils::RandomRange(seed, sp.spawnRadiusMin, sp.spawnRadiusMax);
                glm::vec3 pos(anchorPos.x + dir.x * r, anchorPos.y + dir.y * r, anchorPos.z);

                Engine::Entity npc = SpawnZombie(scene, sp.npcPrefabId, pos);
                if (npc)
                {
                    sp.spawnedSoFar++;

                    // Only track if you want the tracking feature
                    sp.spawned.push_back((Engine::Entity)npc);
                }
            }
        });
}



// You implement this in your game code:
Engine::Entity NpcSpawnControllerSystem::SpawnZombie(Engine::Scene* scene, uint32_t prefabID, const glm::vec2& pos)
{
   


    Engine::NpcPrefab zombie{};

    if (prefabID == 1)
    {
        zombie.name = "Enemy";
        zombie.meshKey = "zombieMesh1";
        zombie.pitchOffsetDeg = 180.0f;
        zombie.viewAngleDeg = 360.0f;

    }

    float variation = 0.5f;
    zombie.moveSpeed = zombie.moveSpeed + Engine::SpawnUtils::RandomFloat(-variation, variation);

    zombie.moveSpeed = 3.0f;
    zombie.radius = 0.30f;

    
    Engine::Entity spawnedEntity = Engine::SpawnUtils::SpawnNPCFromPrefab(scene,zombie, glm::vec3(pos.x, pos.y, 0.0f));
    return spawnedEntity;
}

