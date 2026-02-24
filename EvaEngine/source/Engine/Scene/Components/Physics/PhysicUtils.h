#pragma once
#include "Engine/Scene/Entity.h"
#include "glm/glm.hpp"
#include "PhysicsComponent.h"

class PhysicsUtils{

public:
    static bool PhysicsUtils::AttachSimplePhysics(Engine::Entity e, glm::vec2 initialVelocity,
        float initialSpinRadPerSec, float simulateSeconds, bool destroyOnFinish, glm::vec2 gravity)
    {
        if (e.HasComponent<PhysicsComponent>())
            return false;


        auto& pc = e.AddComponent<PhysicsComponent>();
        pc.velocity = initialVelocity;
        pc.angularVelocity = initialSpinRadPerSec;
        pc.duration = simulateSeconds;
        pc.timeLeft = simulateSeconds;
        pc.gravity = gravity;
        pc.active = true;
        pc.destroyOnFinish = destroyOnFinish;

        return true;
    }

    static Engine::Entity PhysicsUtils::DetachTileAsPhysicsEntity(
        Engine::Scene* scene,
        Engine::Entity sourceEntity,
        uint32_t tileIndex,
        const glm::vec3& impulseVelocity)
    {
        auto& sourceTile = sourceEntity.GetComponent<Engine::TileComponent>();
        auto& sourceTr = sourceEntity.GetComponent<Engine::TransformComponent>();

        EE_ASSERT(tileIndex < sourceTile.tiles.size(), "Invalid tile index");

        Engine::TileInfo tile = sourceTile.tiles[tileIndex]; // copy the tile data

        // Remove from source (swap-erase to avoid shifting indices)
        sourceTile.tiles[tileIndex] = sourceTile.tiles.back();
        sourceTile.tiles.pop_back();

        // Spawn a new single-tile entity
        Engine::Entity gibEntity = scene->CreateEntity("PhysicsTile");

        Engine::TransformComponent& tr = gibEntity.AddComponent<Engine::TransformComponent>();
        tr.Translation = sourceTr.Translation;
        tr.Rotation = sourceTr.Rotation;
        tr.Scale = sourceTr.Scale;

        Engine::TileComponent& tileComp = gibEntity.AddComponent<Engine::TileComponent>();
        tileComp.tiles.push_back(tile);

        PhysicsComponent& phys = gibEntity.AddComponent<PhysicsComponent>();
        phys.velocity = impulseVelocity;
        phys.gravity = glm::vec3(0.0f, -9.8f, 0.0f);
        phys.active = true;
        phys.removeOnFinish = false;
        phys.destroyOnFinish = true; // destroy the whole entity when done
        phys.duration = 0.4f;
        phys.timeLeft = 0.5f;
        phys.randomizedSpin = false;

        return gibEntity;
    }


};


