#pragma once

#include <box2d/box2d.h>
#include <glm/glm.hpp>
#include <Engine/Scene/Component.h>

namespace Engine
{
    class Scene;
    class Entity;

    struct TransformComponent;
    struct RigidBody2DComponent;
    struct CircleCollider2DComponent;

    class Box2DPhysicsSystem
    {
    public:
        Box2DPhysicsSystem() = default;
        ~Box2DPhysicsSystem() = default;


        void Init();
        void Shutdown();

        void OnRuntimeStart(Scene* scene);
        void OnRuntimeStop(Scene* scene);

        void Step(Scene* scene, float dt, int subSteps = 4);

        void CreatePhysicsBody(Entity entity);
        void DestroyPhysicsBody(Entity entity);

        void SyncPhysicsToTransforms(Scene* scene);
        void SyncTransformToPhysics(Entity entity);

        void SetLinearVelocity(Entity entity, const glm::vec2& velocity);
        glm::vec2 GetLinearVelocity(Entity entity) const;

        void SetTransform(Entity entity, const glm::vec2& position, float angleRadians);
        glm::vec2 GetPosition(Entity entity) const;
        float GetAngle(Entity entity) const;

        b2WorldId GetWorldId() const { return m_WorldId; }
        bool IsWorldValid() const { return b2World_IsValid(m_WorldId); }


    private:

        b2BodyType ToB2BodyType(RigidBody2DComponent::BodyType bodyType) const;
        void CreateCircleShape(Entity entity, b2BodyId bodyId);

    private:
        Scene* m_Scene = nullptr;
        b2WorldId m_WorldId = b2_nullWorldId;
    };
}