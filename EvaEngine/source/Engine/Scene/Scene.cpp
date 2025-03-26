#include "pch.h"
#include "Scene.h"
#include "Engine.h"

#include <glm/glm.hpp>

#include "Component.h"
#include "ScriptableEntity.h"

#include "box2d/box2d.h"

//#include "box2d/collision.h"
//#include "box2d/types.h"
//#include "box2d/base.h"
//#include "box2d/id.h"
#include "box2d/math_functions.h"


namespace Engine {


    entt::entity Scene::GetEntityByUUID(entt::registry& registry, Engine::UUID uuid) 
    {
        auto view = registry.view<IDComponent>();

        for (auto entity : view)
        {
            auto& idComponent = view.get<IDComponent>(entity);
            if (idComponent.ID == uuid) {
                return entity;
            }
        }

        return entt::null;  // Return null if no matching entity is found
    }

    static b2BodyType Rigidbody2dTypeToBox2D(RigidBody2DComponent::BodyType bodytype)
    {
        switch (bodytype)
        {
            case Engine::RigidBody2DComponent::BodyType::Static:
            {
                return b2BodyType::b2_staticBody;
            }
            case Engine::RigidBody2DComponent::BodyType::Dynamic:
            {
                return b2BodyType::b2_dynamicBody;
            }
            case Engine::RigidBody2DComponent::BodyType::Kinematic:
            {
                return b2BodyType::b2_kinematicBody;

            }
        }
        EE_CORE_ASSERT(false, " unkown bodytype");
        return b2BodyType::b2_staticBody;
    }

    Scene::Scene()
    {

        m_registry = entt::registry();
        
    }


    Scene::~Scene()
    {
         
    }

    
    template<typename Component>
    static void CopyComponent(entt::registry& dst, entt::registry& src, const std::unordered_map<UUID, entt::entity>& enttMap)
    {
        auto view = src.view<Component>();
        for (auto e : view)
        {
            UUID uuid = src.get<IDComponent>(e).ID;
            EE_CORE_ASSERT(enttMap.find(uuid) != enttMap.end());

            entt::entity dstEnttID = enttMap.at(uuid);

            auto& component = src.get<Component>(e);
            dst.emplace_or_replace<Component>(dstEnttID, component);

        }

    }
    
    template<typename Component>
    static void CopyComponentIfExists(Entity dstEntity, Entity srcEntity)
    {
        if (srcEntity.HasComponent<Component>())
        {
            dstEntity.AddOrReplaceComponent<Component>(srcEntity.GetComponent<Component>());
        }
    }
    
    Ref<Scene> Scene::Copy(Ref<Scene> other)
    {
        Ref<Scene> newScene = std::make_shared<Scene>();

        newScene->m_viewportWidth = other->m_viewportWidth;
        newScene->m_viewportHeight = other->m_viewportHeight;

        std::unordered_map<UUID, entt::entity> enttMap;

        auto& srcSceneRegistry = other->m_registry;
        auto& dstSceneRegistry = newScene->m_registry;
        auto idView = srcSceneRegistry.view<IDComponent>();
        for (auto e : idView)
        {
            UUID uuid = srcSceneRegistry.get<IDComponent>(e).ID;
            const auto& name = srcSceneRegistry.get<TagComponent>(e).Tag;
            Entity newEntity = newScene->CreateEntityWithUUID(uuid, name);

            enttMap[uuid] = (entt::entity)newEntity;

        }
        
        CopyComponent<TransformComponent>(dstSceneRegistry, srcSceneRegistry, enttMap);
        CopyComponent<SpriteRendererComponent>(dstSceneRegistry, srcSceneRegistry, enttMap);
        CopyComponent<CameraComponent>(dstSceneRegistry, srcSceneRegistry, enttMap);
        CopyComponent<BoxCollider2DComponent>(dstSceneRegistry, srcSceneRegistry, enttMap);
        CopyComponent<RigidBody2DComponent>(dstSceneRegistry, srcSceneRegistry, enttMap);
        CopyComponent<NativeScriptComponent>(dstSceneRegistry, srcSceneRegistry, enttMap);
        CopyComponent<CircleRendererComponent>(dstSceneRegistry, srcSceneRegistry, enttMap);
        CopyComponent<CircleCollider2DComponent>(dstSceneRegistry, srcSceneRegistry, enttMap);
        
        return newScene;
    }
    

    void Scene::CopyEntities(Ref<Scene> sourceScene, Ref<Scene> combinedScene, std::unordered_map<UUID, entt::entity>& enttMap)
    {
        auto& srcSceneRegistry = sourceScene->m_registry;
        auto& dstSceneRegistry = combinedScene->m_registry;
        auto idView = srcSceneRegistry.view<IDComponent>();

        std::vector<entt::entity> entitiesToCopy; // Store entities to copy components later

        for (auto e : idView)
        {
            UUID uuid = srcSceneRegistry.get<IDComponent>(e).ID;

            // If the entity already exists in enttMap, skip it
            if (enttMap.find(uuid) != enttMap.end())
                continue;

            // Otherwise, create a new entity and store it in enttMap
            const auto& name = srcSceneRegistry.get<TagComponent>(e).Tag;
            Entity newEntity = combinedScene->CreateEntityWithUUID(uuid, name);
            enttMap[uuid] = (entt::entity)newEntity;

            // Store the entity for component copying
            entitiesToCopy.push_back(e);
        }

        // Now, copy components only for the entities that were actually added
        for (auto e : entitiesToCopy)
        {
            UUID uuid = srcSceneRegistry.get<IDComponent>(e).ID;
            entt::entity dstEnttID = enttMap.at(uuid); // Now guaranteed to exist

            CopyComponent<TransformComponent>(dstSceneRegistry, srcSceneRegistry, enttMap);
            CopyComponent<SpriteRendererComponent>(dstSceneRegistry, srcSceneRegistry, enttMap);
            CopyComponent<CameraComponent>(dstSceneRegistry, srcSceneRegistry, enttMap);
            CopyComponent<BoxCollider2DComponent>(dstSceneRegistry, srcSceneRegistry, enttMap);
            CopyComponent<RigidBody2DComponent>(dstSceneRegistry, srcSceneRegistry, enttMap);
            CopyComponent<NativeScriptComponent>(dstSceneRegistry, srcSceneRegistry, enttMap);
            CopyComponent<CircleRendererComponent>(dstSceneRegistry, srcSceneRegistry, enttMap);
            CopyComponent<CircleCollider2DComponent>(dstSceneRegistry, srcSceneRegistry, enttMap);
        }
    }




    Ref<Scene> Scene::Combine(Ref<Scene> sceneA, Ref<Scene> sceneB)
    {
        Ref<Scene> combinedScene = std::make_shared<Scene>();

        combinedScene->m_viewportWidth = sceneA->m_viewportWidth;
        combinedScene->m_viewportHeight = sceneA->m_viewportHeight;

        std::unordered_map<UUID, entt::entity> enttMap;

        CopyEntities(sceneA, combinedScene, enttMap);
        CopyEntities(sceneB, combinedScene, enttMap);

        return combinedScene;
    }



    Entity Scene::CreateEntity(const std::string& name)
    {

        return CreateEntityWithUUID(UUID(), name);

    }

    Entity Scene::CreateEntityWithUUID(UUID uuid, const std::string& name)
    {
        Entity entity = { m_registry.create(), this };
        entity.AddComponent<IDComponent>(uuid);
        auto tag = entity.AddComponent<TagComponent>(std::move(name.empty() ? "Entity" : name));
        return entity;
    }

    bool Scene::DestroyEntity(Entity entity)
    {
        if (m_registry.valid(entity))
        {
            m_registry.destroy(entity);
            return true;
        }

        EE_CORE_WARN("Tried to destroy an invalid entity!");
        return false;
    }

    void Scene::OnRunTimeStart()
    {
        b2WorldDef world = b2DefaultWorldDef();
        
        b2Vec2 gravity{};
        gravity.x = 0.0f;
        gravity.y = -9.8f;
        world.gravity = gravity;
        

        m_worldId = b2CreateWorld(&world);
       

        auto view = m_registry.view<RigidBody2DComponent>();
        for (auto e : view)
        {
            Entity entity = { e, this };
            auto& transformComp = entity.GetComponent<TransformComponent>();
            auto& rb2dComp = entity.GetComponent<RigidBody2DComponent>();

            b2BodyDef bodyDef = b2DefaultBodyDef();
            bodyDef.type = Rigidbody2dTypeToBox2D(rb2dComp.Type);
            b2Vec2 position;
            position.x = transformComp.Translation.x;
            position.y = transformComp.Translation.y;
            bodyDef.position = position;

            float angle = transformComp.Rotation.z; // Assuming Rotation.z holds the rotation in radians
            bodyDef.rotation.c = std::cos(angle);
            bodyDef.rotation.s = std::sin(angle);
            
        
            bodyDef.fixedRotation = rb2dComp.FixedRotation;
            
            EE_CORE_ASSERT(bodyId != b2_nullBodyId);  // Make sure the body is created successfully.
            b2BodyId bodyId = b2CreateBody(m_worldId, &bodyDef);

            rb2dComp.RuntimeBody = bodyId;

            if (entity.HasComponent<BoxCollider2DComponent>())
            {
                auto& colliderComp = entity.GetComponent<BoxCollider2DComponent>();
                b2ShapeDef shapeDef = b2DefaultShapeDef();
                shapeDef.density = colliderComp.Density;
                shapeDef.friction = colliderComp.Friction;
                shapeDef.restitution = colliderComp.Restitution;

                b2Polygon dynamicBox = b2MakeBox(colliderComp.Size.x * transformComp.Scale.x , colliderComp.Size.y * transformComp.Scale.y);
                
                b2ShapeId boxShapeID = b2CreatePolygonShape(bodyId, &shapeDef, &dynamicBox);
                colliderComp.shapeID = boxShapeID;
            }
            if(entity.HasComponent<CircleCollider2DComponent>())
            {
                auto& colliderComp = entity.GetComponent<CircleCollider2DComponent>();

                b2ShapeDef shapeDef = b2DefaultShapeDef();
                shapeDef.density = colliderComp.Density;
                shapeDef.friction = colliderComp.Friction;
                shapeDef.restitution = colliderComp.Restitution;

                // Define a circle shape (instead of using an AABB)
                b2Circle circleShape;
                circleShape.radius = colliderComp.Radius * transformComp.Scale.x;
                b2Vec2 center;
                center.x = colliderComp.Offset.x;
                center.y = colliderComp.Offset.y;
                circleShape.center = center;

                b2ShapeId circleShapeID = b2CreateCircleShape(bodyId, &shapeDef, &circleShape);
                colliderComp.shapeID = circleShapeID;
            }

        }


    }

    void Scene::OnRunTimeStop()
    {
        b2DestroyWorld(m_worldId);
    }

    void Scene::PauseRuntime()
    {
        b2BodyEvents bodyEvents = b2World_GetBodyEvents(m_worldId);

        // TODO save all velocities and add them on resume
    }


    void Scene::ResumeRuntime()
    {

    }

    void Scene::OnUpdateRuntime(Timestep timestep, bool isPlaying)
    {

        EE_PROFILE_FUNCTION();
        /*
        ╔════════════════════════════════════════════════╗
        ║ 🚀 EVA ENGINE | ENTT                        	║
        ║                                                ║
        ╚════════════════════════════════════════════════╝
        // Full-owning group: The registry owns and tightly packs both SpriteRendererComponent and TransformComponent
        auto group = m_registry.group<SpriteRendererComponent, TransformComponent>();
        ✅ Pros: Fastest iteration speed, best memory locality.
        ❌ Cons: Less flexibility, requires full ownership.

        // Partial-owning group: Owns SpriteRendererComponent but references TransformComponent without owning it
        auto group = m_registry.group<SpriteRendererComponent>(entt::get<TransformComponent>);
        ✅ Pros: Keeps SpriteRendererComponent tightly packed, while still accessing TransformComponent.
        ❌ Cons: TransformComponent is looked up dynamically, adding slight overhead.

        // Non-owning group: Does not own any components, just filters entities that have both components
        auto group = m_registry.group<>(entt::get<SpriteRendererComponent, TransformComponent>);
        ✅ Pros: No memory reordering, keeps components untouched.
        ❌ Cons: Slightly slower than owning groups because it doesn’t pack memory efficiently.


        */




        // update scripts
        {
            m_registry.view<NativeScriptComponent>().each([=](auto entity, auto& nsc)
                {

                    if (!nsc.Instance)
                    {
                        nsc.Instance = nsc.InstantiateScript();
                        nsc.Instance->m_entity = Entity{ entity, this };
                        nsc.Instance->OnCreate();
                    }

                    nsc.Instance->OnUpdate(timestep);

                });
        }




        // physics
        if (isPlaying)
        {

            UpdatePhysics(timestep);
        }

        Camera* mainCamera = nullptr;
        glm::mat4 cameraTransform;
        {
            EE_PROFILE_SCOPE("Get Update Runtime Camera");

            {
                auto group = m_registry.group<TransformComponent, CameraComponent>();
                for (auto entity : group)
                {
                    auto [transform, camera] = group.get<TransformComponent, CameraComponent>(entity);

                    if (camera.Primary)
                    {
                        mainCamera = &camera.Camera;
                        cameraTransform = transform.GetTransform();
                        break;
                    }
                }
            }
        }

        if(mainCamera)
        {   
            Renderer2D::BeginScene(mainCamera->GetViewProjection(), cameraTransform);
            

            {
                EE_PROFILE_SCOPE("Update Runtime CircleRendererComponent");

                auto view = m_registry.view<CircleRendererComponent, TransformComponent>();

                for (auto entity : view)
                {
                    auto [transform, circle] = view.get<TransformComponent, CircleRendererComponent>(entity);

                    Renderer2D::DrawCircle(transform.GetTransform(), circle.Color, circle.Thickness, circle.Fade, (int)entity);
                }
            }
            {
                EE_PROFILE_SCOPE("Update Runtime SpriteRendererComponent");

                // Define the container to hold the instance transforms
                std::vector<glm::mat4> instanceTransforms;
                std::vector<glm::vec4> instanceColors;
                std::vector<int> instanceTextureIDs;

                // Iterate through each entity in the view
                auto view = m_registry.view<SpriteRendererComponent, TransformComponent>();
                for (auto entity : view)
                {
                    // Get the transform and sprite components for the entity
                    auto [transform, sprite] = view.get<TransformComponent, SpriteRendererComponent>(entity);

                    // Collect the instance data for the instanced draw call
                    instanceTransforms.push_back(transform.GetTransform());  // Transformation matrix for the entity
                    instanceColors.push_back(sprite.Color);  // Color for the sprite
                }

                // After the loop, make a single instanced draw call
                if (!instanceTransforms.empty())
                {
                    // Pass all collected instance data in one call
                    Renderer2D::DrawQuadInstanced(glm::mat4(1.0f), glm::vec4(1.0f), 0, instanceTransforms);  // You may want to adjust parameters (like the color or texture) here
                }

            }

            Renderer2D::EndScene();
        }


    }

    void Scene::OnUpdateEditor(Timestep timestep, EditorCamera& camera)
    {
        Renderer2D::BeginScene(camera);

        {
            auto view = m_registry.view<SpriteRendererComponent, TransformComponent>();

            for (auto entity : view)
            {
                auto [transform, sprite] = view.get<TransformComponent, SpriteRendererComponent>(entity);

                Renderer2D::DrawSprite(transform.GetTransform(), sprite, (int)entity);

            }
        }

        {
            auto view = m_registry.view<CircleRendererComponent, TransformComponent>();

            for (auto entity : view)
            {
                auto [transform, circle] = view.get<TransformComponent, CircleRendererComponent>(entity);

                Renderer2D::DrawCircle(transform.GetTransform(), circle.Color, circle.Thickness, circle.Fade, (int)entity);

            }
        }

        Renderer2D::EndScene();
    }

    void Scene::OnViewportResize(uint32_t width, uint32_t height)
    {
        m_viewportHeight = height;
        m_viewportWidth = width;

        auto view = m_registry.view<CameraComponent>();

        for (auto entity : view)
        {
            auto cameraComp = view.get<CameraComponent>(entity);
            if (!cameraComp.FixedAspectRatio)
            {
                cameraComp.Camera.SetViewportSize(width, height);
            }

        }


    }

    void Scene::DuplicateEntity(Entity entity)
    {
        Entity newEntity = CreateEntity(entity.GetName());

        CopyComponentIfExists<TransformComponent>(newEntity, entity);
        CopyComponentIfExists<SpriteRendererComponent>(newEntity, entity);
        CopyComponentIfExists<CameraComponent>(newEntity, entity);
        CopyComponentIfExists<BoxCollider2DComponent>(newEntity, entity);
        CopyComponentIfExists<RigidBody2DComponent>(newEntity, entity);
        CopyComponentIfExists<NativeScriptComponent>(newEntity, entity);
        CopyComponentIfExists<CircleRendererComponent>(newEntity, entity);
        CopyComponentIfExists<CircleCollider2DComponent>(newEntity, entity);

    }





    Entity Scene::GetPrimaryCameraEntity()
    {
        auto view = m_registry.view<CameraComponent>();
        for (auto cameraEntity : view)
        {
            const auto& cameraComp = view.get<CameraComponent>(cameraEntity);

            if (cameraComp.Primary)
            {
                return Entity{ cameraEntity, this };
            }
        }
        return {};
    }

    void Scene::UpdatePhysics(Timestep timestep)
    {
        EE_PROFILE_FUNCTION();

        const int32_t subStepCount = 4;
        float physicsStep = 1.0f / 60.0f;

        // update physics
        b2World_Step(m_worldId, physicsStep, subStepCount);
        auto view = m_registry.view<RigidBody2DComponent>();
        for (auto e : view)
        {
            Entity entity = { e, this };
            TransformComponent& transformComp = entity.GetComponent<TransformComponent>();
            auto& rb2dComp = entity.GetComponent<RigidBody2DComponent>();

            b2BodyId bodyId = rb2dComp.RuntimeBody;

            b2Vec2 position = b2Body_GetPosition(bodyId);
            transformComp.Translation = { position.x, position.y, 0.0f };

            b2Rot rotation = b2Body_GetRotation(bodyId);
            transformComp.Rotation.z = std::atan2(rotation.s, rotation.c);

        }
    }




    template<typename T>
    inline void Scene::OnComponentAdded(Entity entity, T& component)
    {
        //  fails at compile-time if there’s no explicit specialization for a given component type.
       static_assert(false);

    }

    // probably remove this. or remove static_assert ^
    // Specializations for Specific Components 
    template<>
    void Scene::OnComponentAdded<TransformComponent>(Entity entity, TransformComponent& component)
    {

    }

    template<>
    void Scene::OnComponentAdded<CameraComponent>(Entity entity, CameraComponent& component)
    {
        component.Camera.SetViewportSize(m_viewportWidth, m_viewportHeight);
    }

    template<>
    void Scene::OnComponentAdded<SpriteRendererComponent>(Entity entity, SpriteRendererComponent& component)
    {

    }

    template<>
    void Scene::OnComponentAdded<CircleRendererComponent>(Entity entity, CircleRendererComponent& component)
    {

    }

    template<>
    void Scene::OnComponentAdded<NativeScriptComponent>(Entity entity, NativeScriptComponent& component)
    {

    }

    template<>
    void Scene::OnComponentAdded<TagComponent>(Entity entity, TagComponent& component)
    {

    }

    template<>
    void Scene::OnComponentAdded<RigidBody2DComponent>(Entity entity, RigidBody2DComponent& component)
    {

    }

    template<>
    void Scene::OnComponentAdded<BoxCollider2DComponent>(Entity entity, BoxCollider2DComponent& component)
    {

    }

    template<>
    void Scene::OnComponentAdded<CircleCollider2DComponent>(Entity entity, CircleCollider2DComponent& component)
    {

    }

    template<>
    void Scene::OnComponentAdded<IDComponent>(Entity entity, IDComponent& component)
    {

    }
}
