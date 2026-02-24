#pragma once
#include <Engine/Scene/Scene.h>
#include "glm/glm.hpp"
#include "Engine/Scene/Entity.h"
#include <Engine/Scene/Components/Physics/PhysicsComponent.h>

class PhysicsUtils
{

public:
    static void AttachSimplePhysics(Engine::Entity e, glm::vec2 initialVelocity,
        float initialSpinRadPerSec, float simulateSeconds, bool destroyOnFinish, glm::vec2 gravity)
    {
        auto& pc = e.AddComponent<PhysicsComponent>();
        pc.velocity = initialVelocity;
        pc.angularVelocity = initialSpinRadPerSec;
        pc.duration = simulateSeconds;
        pc.timeLeft = simulateSeconds;
        pc.gravity = gravity;
        pc.active = true;
        pc.destroyOnFinish = destroyOnFinish;
    }


};
