#include "pch.h"
#include "Box2DPhysicsSystem.h"

#include "Engine/Scene/Scene.h"
#include "Engine/Scene/Entity.h"

namespace Engine
{


   

    // Init / Shutdown

    void Box2DPhysicsSystem::Init()
    {
        if (b2World_IsValid(m_WorldId))
            return;

        b2WorldDef worldDef = b2DefaultWorldDef();
        worldDef.gravity = b2Vec2{ 0.0f, 0.0f }; // top-down game

        m_WorldId = b2CreateWorld(&worldDef);
    }

    void Box2DPhysicsSystem::Shutdown()
    {
        if (b2World_IsValid(m_WorldId))
        {
            b2DestroyWorld(m_WorldId);
            m_WorldId = b2_nullWorldId;
        }

        m_Scene = nullptr;
    }

    // Runtime 

    void Box2DPhysicsSystem::OnRuntimeStart(Scene* scene)
    {
        if (!scene)
            return;

        m_Scene = scene;
        Init();

        scene->ForEach<TransformComponent, RigidBody2DComponent>(
            [this](Entity e, TransformComponent& transform, RigidBody2DComponent& rb)
            {
                CreatePhysicsBody(e);
            });
    }

    void Box2DPhysicsSystem::OnRuntimeStop(Scene* scene)
    {
        if (!scene)
            return;

        scene->ForEach<RigidBody2DComponent>(
            [this](Entity e, RigidBody2DComponent& rb)
            {
                DestroyPhysicsBody(e);
            });

        Shutdown();
    }

    // Body

    void Box2DPhysicsSystem::CreatePhysicsBody(Entity entity)
    {
        if (!entity)
            return;

        if (!b2World_IsValid(m_WorldId))
            return;

        if (!entity.HasComponent<TransformComponent>() || !entity.HasComponent<RigidBody2DComponent>())
            return;

        TransformComponent& transform = entity.GetComponent<TransformComponent>();
        RigidBody2DComponent& rb = entity.GetComponent<RigidBody2DComponent>();

        if (b2Body_IsValid(rb.BodyId))
            return;

        b2BodyDef bodyDef = b2DefaultBodyDef();
        bodyDef.type = ToB2BodyType(rb.Type);
        bodyDef.position = b2Vec2{ transform.Translation.x, transform.Translation.y };
        bodyDef.rotation = b2MakeRot(transform.Rotation.z);
        bodyDef.fixedRotation = rb.FixedRotation;
        bodyDef.isBullet = rb.IsBullet;

        rb.BodyId = b2CreateBody(m_WorldId, &bodyDef);

        if (entity.HasComponent<CircleCollider2DComponent>())
        {
            CreateCircleShape(entity, rb.BodyId);
        }
    }

    void Box2DPhysicsSystem::CreateCircleShape(Entity entity, b2BodyId bodyId)
    {
        if (!entity.HasComponent<CircleCollider2DComponent>())
            return;

        CircleCollider2DComponent& circle = entity.GetComponent<CircleCollider2DComponent>();

        if (b2Shape_IsValid(circle.ShapeId))
            return;

        b2ShapeDef shapeDef = b2DefaultShapeDef();
        shapeDef.density = circle.Density;
        shapeDef.friction = circle.Friction;
        shapeDef.restitution = circle.Restitution;
        
        shapeDef.isSensor = circle.IsSensor;

        b2Circle b2circle{};
        b2circle.center = b2Vec2{ circle.Offset.x, circle.Offset.y };
        b2circle.radius = circle.Radius;

        circle.ShapeId = b2CreateCircleShape(bodyId, &shapeDef, &b2circle);
    }

    void Box2DPhysicsSystem::DestroyPhysicsBody(Entity entity)
    {
        if (!entity)
            return;

        if (!entity.HasComponent<RigidBody2DComponent>())
            return;

        RigidBody2DComponent& rb = entity.GetComponent<RigidBody2DComponent>();

        if (entity.HasComponent<CircleCollider2DComponent>())
        {
            CircleCollider2DComponent& circle = entity.GetComponent<CircleCollider2DComponent>();

            if (b2Shape_IsValid(circle.ShapeId))
            {
                b2DestroyShape(circle.ShapeId, true);
                circle.ShapeId = b2_nullShapeId;
            }
        }

        if (b2Body_IsValid(rb.BodyId))
        {
            b2DestroyBody(rb.BodyId);
            rb.BodyId = b2_nullBodyId;
        }
    }

    // Simulation
    
    void Box2DPhysicsSystem::Step(Scene* scene, float dt, int subSteps)
    {
        EE_PROFILE_FUNCTION();
        if (!scene)
            return;

        if (!b2World_IsValid(m_WorldId))
            return;

        if (dt <= 0.0f)
            return;

        b2World_Step(m_WorldId, dt, subSteps);

        SyncPhysicsToTransforms(scene);
    }

    // Sync

    void Box2DPhysicsSystem::SyncPhysicsToTransforms(Scene* scene)
    {
        if (!scene)
            return;

        scene->ForEach<TransformComponent, RigidBody2DComponent>(
            [](Entity e, TransformComponent& transform, RigidBody2DComponent& rb)
            {
                if (!b2Body_IsValid(rb.BodyId))
                    return;

                b2Vec2 pos = b2Body_GetPosition(rb.BodyId);
                //b2Rot rot = b2Body_GetRotation(rb.BodyId);

                transform.Translation.x = pos.x;
                transform.Translation.y = pos.y;
              //  transform.Rotation.z = b2Rot_GetAngle(rot);
            });
    }

    void Box2DPhysicsSystem::SyncTransformToPhysics(Entity entity)
    {
        if (!entity)
            return;

        if (!entity.HasComponent<TransformComponent>() || !entity.HasComponent<RigidBody2DComponent>())
            return;

        TransformComponent& transform = entity.GetComponent<TransformComponent>();
        RigidBody2DComponent& rb = entity.GetComponent<RigidBody2DComponent>();

        if (!b2Body_IsValid(rb.BodyId))
            return;

        b2Body_SetTransform(rb.BodyId, b2Vec2{ transform.Translation.x, transform.Translation.y },
            b2MakeRot(transform.Rotation.z)
        );
    }

    // Gameplay helpers

    b2BodyType Box2DPhysicsSystem::ToB2BodyType(RigidBody2DComponent::BodyType bodyType) const
    {
        switch (bodyType)
        {
        case RigidBody2DComponent::BodyType::Static:
            return b2_staticBody;

        case RigidBody2DComponent::BodyType::Dynamic:
            return b2_dynamicBody;

        case RigidBody2DComponent::BodyType::Kinematic:
            return b2_kinematicBody;
        }

        return b2_dynamicBody;
    }

    void Box2DPhysicsSystem::SetLinearVelocity(Entity entity, const glm::vec2& velocity)
    {
        if (!entity || !entity.HasComponent<RigidBody2DComponent>())
            return;

        RigidBody2DComponent& rb = entity.GetComponent<RigidBody2DComponent>();

        if (!b2Body_IsValid(rb.BodyId))
            return;

        b2Body_SetLinearVelocity(rb.BodyId, b2Vec2{ velocity.x, velocity.y });
    }

    glm::vec2 Box2DPhysicsSystem::GetLinearVelocity(Entity entity) const
    {
        if (!entity || !entity.HasComponent<RigidBody2DComponent>())
            return glm::vec2(0.0f);

        const RigidBody2DComponent& rb = entity.GetComponent<RigidBody2DComponent>();

        if (!b2Body_IsValid(rb.BodyId))
            return glm::vec2(0.0f);

        b2Vec2 vel = b2Body_GetLinearVelocity(rb.BodyId);
        return glm::vec2(vel.x, vel.y);
    }

    void Box2DPhysicsSystem::SetTransform(Entity entity, const glm::vec2& position, float angleRadians)
    {
        if (!entity)
            return;

        if (!entity.HasComponent<RigidBody2DComponent>() || !entity.HasComponent<TransformComponent>())
            return;

        RigidBody2DComponent& rb = entity.GetComponent<RigidBody2DComponent>();
        TransformComponent& transform = entity.GetComponent<TransformComponent>();

        transform.Translation.x = position.x;
        transform.Translation.y = position.y;
        transform.Rotation.z = angleRadians;

        if (b2Body_IsValid(rb.BodyId))
        {
            b2Body_SetTransform(rb.BodyId, b2Vec2{ position.x, position.y }, b2MakeRot(angleRadians));
        }
    }

    glm::vec2 Box2DPhysicsSystem::GetPosition(Entity entity) const
    {
        if (!entity || !entity.HasComponent<RigidBody2DComponent>())
            return glm::vec2(0.0f);

        const RigidBody2DComponent& rb = entity.GetComponent<RigidBody2DComponent>();

        if (!b2Body_IsValid(rb.BodyId))
            return glm::vec2(0.0f);

        b2Vec2 pos = b2Body_GetPosition(rb.BodyId);
        return glm::vec2(pos.x, pos.y);
    }

    float Box2DPhysicsSystem::GetAngle(Entity entity) const
    {
        if (!entity || !entity.HasComponent<RigidBody2DComponent>())
            return 0.0f;

        const RigidBody2DComponent& rb = entity.GetComponent<RigidBody2DComponent>();

        if (!b2Body_IsValid(rb.BodyId))
            return 0.0f;

        b2Rot rot = b2Body_GetRotation(rb.BodyId);
        return b2Rot_GetAngle(rot);
    }

} 