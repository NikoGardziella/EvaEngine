#include "pch.h"
#include "Scene.h"
#include "Engine.h"

#include "Engine/Math/Math.h"
#include "Component.h"
#include "Engine/Scene/Components/Combat/HealthComponent.h"
#include "Engine/Scene/Components/Combat/WeaponComponent.h"
#include <Engine/Scene/Components/Player/CharacterControllerComponent.h>

#include "ScriptableEntity.h"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/string_cast.hpp>
#include <glm/gtx/matrix_decompose.hpp>



#include "box2d/box2d.h"
#include "box2d/math_functions.h"
#include "Components/NPC/NpcAIComponent.h"



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
		newScene->m_viewportBounds[0] = other->m_viewportBounds[0];

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
        
        CopyAllComponents(dstSceneRegistry, srcSceneRegistry, enttMap);
        /*
        CopyComponent<TransformComponent>(dstSceneRegistry, srcSceneRegistry, enttMap);
        CopyComponent<SpriteRendererComponent>(dstSceneRegistry, srcSceneRegistry, enttMap);
        CopyComponent<CameraComponent>(dstSceneRegistry, srcSceneRegistry, enttMap);
        CopyComponent<BoxCollider2DComponent>(dstSceneRegistry, srcSceneRegistry, enttMap);
        CopyComponent<RigidBody2DComponent>(dstSceneRegistry, srcSceneRegistry, enttMap);
        CopyComponent<NativeScriptComponent>(dstSceneRegistry, srcSceneRegistry, enttMap);
        CopyComponent<CircleRendererComponent>(dstSceneRegistry, srcSceneRegistry, enttMap);
        CopyComponent<CircleCollider2DComponent>(dstSceneRegistry, srcSceneRegistry, enttMap);
        CopyComponent<CharacterControllerComponent>(dstSceneRegistry, srcSceneRegistry, enttMap);
        CopyComponent<ProjectileComponent>(dstSceneRegistry, srcSceneRegistry, enttMap);
        CopyComponent<HealthComponent>(dstSceneRegistry, srcSceneRegistry, enttMap);

        */

        newScene->m_gameplaySystems = other->m_gameplaySystems;

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

            CopyAllComponents(dstSceneRegistry, srcSceneRegistry, enttMap);
            /*
            CopyComponent<TransformComponent>(dstSceneRegistry, srcSceneRegistry, enttMap);
            CopyComponent<SpriteRendererComponent>(dstSceneRegistry, srcSceneRegistry, enttMap);
            CopyComponent<CameraComponent>(dstSceneRegistry, srcSceneRegistry, enttMap);
            CopyComponent<BoxCollider2DComponent>(dstSceneRegistry, srcSceneRegistry, enttMap);
            CopyComponent<RigidBody2DComponent>(dstSceneRegistry, srcSceneRegistry, enttMap);
            CopyComponent<NativeScriptComponent>(dstSceneRegistry, srcSceneRegistry, enttMap);
            CopyComponent<CircleRendererComponent>(dstSceneRegistry, srcSceneRegistry, enttMap);
            CopyComponent<CircleCollider2DComponent>(dstSceneRegistry, srcSceneRegistry, enttMap);
            CopyComponent<CharacterControllerComponent>(dstSceneRegistry, srcSceneRegistry, enttMap);
            CopyComponent<HealthComponent>(dstSceneRegistry, srcSceneRegistry, enttMap);
            CopyComponent<ProjectileComponent>(dstSceneRegistry, srcSceneRegistry, enttMap);
            */
        }
    }




    Ref<Scene> Scene::Combine(Ref<Scene> sceneA, Ref<Scene> sceneB)
    {
        Ref<Scene> combinedScene = std::make_shared<Scene>();

        combinedScene->m_viewportWidth = sceneA->m_viewportWidth;
        combinedScene->m_viewportHeight = sceneA->m_viewportHeight;
        combinedScene->m_viewportBounds[0] = sceneA->m_viewportBounds[0];

        std::unordered_map<UUID, entt::entity> enttMap;

        CopyEntities(sceneA, combinedScene, enttMap);
        CopyEntities(sceneB, combinedScene, enttMap);

        combinedScene->m_gameplaySystems = sceneA->m_gameplaySystems;
        combinedScene->m_gameplaySystems.insert(
            combinedScene->m_gameplaySystems.end(),
            sceneB->m_gameplaySystems.begin(),
            sceneB->m_gameplaySystems.end()
        );

        return combinedScene;
    }


    void Scene::CopyAllComponents(entt::registry& dstSceneRegistry, entt::registry& srcSceneRegistry, const std::unordered_map<UUID, entt::entity>& enttMap)
    {
        CopyComponent<TransformComponent>(dstSceneRegistry, srcSceneRegistry, enttMap);
        CopyComponent<SpriteRendererComponent>(dstSceneRegistry, srcSceneRegistry, enttMap);
        CopyComponent<CameraComponent>(dstSceneRegistry, srcSceneRegistry, enttMap);
        CopyComponent<BoxCollider2DComponent>(dstSceneRegistry, srcSceneRegistry, enttMap);
        CopyComponent<RigidBody2DComponent>(dstSceneRegistry, srcSceneRegistry, enttMap);
        CopyComponent<NativeScriptComponent>(dstSceneRegistry, srcSceneRegistry, enttMap);
        CopyComponent<CircleRendererComponent>(dstSceneRegistry, srcSceneRegistry, enttMap);
        CopyComponent<CircleCollider2DComponent>(dstSceneRegistry, srcSceneRegistry, enttMap);
        CopyComponent<CharacterControllerComponent>(dstSceneRegistry, srcSceneRegistry, enttMap);
        CopyComponent<HealthComponent>(dstSceneRegistry, srcSceneRegistry, enttMap);
        CopyComponent<ProjectileComponent>(dstSceneRegistry, srcSceneRegistry, enttMap);
        CopyComponent<NPCAIMovementComponent>(dstSceneRegistry, srcSceneRegistry, enttMap);
        CopyComponent<NPCAIVisionComponent>(dstSceneRegistry, srcSceneRegistry, enttMap);
        CopyComponent<WeaponComponent>(dstSceneRegistry, srcSceneRegistry, enttMap);
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
        b2Vec2 gravity{};
        gravity.x = 0.0f;
        gravity.y = -9.8f;
        b2WorldDef worldDef = b2DefaultWorldDef();;
        worldDef.workerCount = std::thread::hardware_concurrency(); // Use max available threads
        worldDef.enqueueTask = &PhysicsTaskScheduler::EnqueueTask;
        worldDef.finishTask = &PhysicsTaskScheduler::FinishTask;
        worldDef.userTaskContext = &m_physicsTaskScheduler;
        worldDef.gravity = gravity;

        m_worldId = b2CreateWorld(&worldDef);
       

        auto view = m_registry.view<RigidBody2DComponent>();

        m_renderSOA.Color.reserve(view.size());
        m_renderSOA.InstanceTransforms.reserve(view.size());
        m_renderSOA.BodyIds.reserve(view.size());
        size_t index = 0;
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

            m_renderSOA.InstanceTransforms.push_back(transformComp.GetTransform());

            
            m_renderSOA.BodyIds.push_back(bodyId);
            if (entity.HasComponent<SpriteRendererComponent>())
            {
                auto& spriteComp = entity.GetComponent<SpriteRendererComponent>();

                m_renderSOA.Color.push_back(spriteComp.Color);
            }
            else
            {
                m_renderSOA.Color.push_back(glm::vec4(1.0f, 1.0f, 1.0f, 1.0f));

            }
            index++;
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
        ║ 🚀 EVA ENGINE | ENTT                           ║
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

        //********** Update all systems **************
        {
            for (auto& system : m_gameplaySystems)
            {
                system(m_registry, timestep, this);
            }
        }


        //************ update scripts ***************
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

        // ******** update physics ************
        if (isPlaying)
        {
            UpdatePhysics(timestep);
        }

        //*********** Render ************

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

            //Renderer2D::BeginScene(mainCamera->GetViewProjection(), cameraTransform);
            Engine::VulkanRenderer2D::BeginScene(mainCamera->GetViewProjection(), cameraTransform);



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
                auto view = m_registry.view<SpriteRendererComponent, TransformComponent>();

                for (auto entity : view)
                {
                    auto [transform, quadSprite] = view.get<TransformComponent, SpriteRendererComponent>(entity);
                        
                    //Engine::VulkanRenderer2D::DrawQuad(transform.GetTransform(), quadSprite.Color);
                }

                std::vector<int> instanceTextureIDs;
                /*
                size_t maxInstances = 600;
                std::vector<glm::mat4> instanceTransforms;
                instanceTransforms.reserve(maxInstances);

                std::vector<glm::vec4> instanceColors;
                instanceColors.reserve(maxInstances);
                
                // Iterate through each entity in the view
                auto view = m_registry.view<const TransformComponent, SpriteRendererComponent>();
                view.each([&](const TransformComponent &transform, const SpriteRendererComponent &sprite)
                    {
                        instanceTransforms.push_back(transform.GetTransform());
                        instanceColors.push_back(sprite.Color);
                    });

                if (instanceTransforms.size() > maxInstances)
                {
                    // increase masxInstances
                    EE_CORE_INFO(" Max instance count reached: {0}", instanceTransforms.size());
                }
                */

                //if (!instanceTransforms.empty())
                {
                    // Pass all collected instance data in one call
                   // Renderer2D::DrawQuadInstanced(m_renderSOA.InstanceTransforms, m_renderSOA.Color, instanceTextureIDs);
                }

            }

            
            {
                EE_PROFILE_SCOPE("Update Runtime PixelSpriteRendererComponent");
                auto view = m_registry.view<PixelSpriteRendererComponent, TransformComponent>();

                for (auto entity : view)
                {
                    auto [transform, quadSprite] = view.get<TransformComponent, PixelSpriteRendererComponent>(entity);

                    float tiling = 1.0f;
                    Engine::VulkanRenderer2D::DrawTextureQuad(transform.GetTransform(), quadSprite.Texture, tiling, quadSprite.Color);
                }

            }

            {
                EE_PROFILE_SCOPE("Update Runtime PixelSpriteRendererComponent");
                auto view = m_registry.view<SpriteRendererComponent, TransformComponent>();

                for (auto entity : view)
                {
                    auto [transform, quadSprite] = view.get<TransformComponent, SpriteRendererComponent>(entity);

                    float tiling = 1.0f;
                    Engine::VulkanRenderer2D::DrawTextureQuad(transform.GetTransform(), quadSprite.Texture, tiling, quadSprite.Color);
                }

            }

            //Engine::Renderer::DrawFrame();
            Engine::VulkanRenderer2D::EndScene();


        }


    }

    void Scene::OnUpdateEditor(Timestep timestep, EditorCamera& camera)
    {
        EE_PROFILE_FUNCTION();

        Engine::VulkanRenderer2D::BeginScene(camera);

        {
            auto view = m_registry.view<SpriteRendererComponent, TransformComponent>();

            for (auto entity : view)
            {
                auto [transform, sprite] = view.get<TransformComponent, SpriteRendererComponent>(entity);

                //Engine::VulkanRenderer2D::DrawQuad(transform.GetTransform(), sprite.Color);

               // Renderer2D::DrawSprite(transform.GetTransform(), sprite, (int)entity);

            }
        }

        {
            auto view = m_registry.view<CircleRendererComponent, TransformComponent>();

            for (auto entity : view)
            {
                auto [transform, circle] = view.get<TransformComponent, CircleRendererComponent>(entity);

               // Renderer2D::DrawCircle(transform.GetTransform(), circle.Color, circle.Thickness, circle.Fade, (int)entity);

            }
        }

		{
			auto view = m_registry.view<PixelSpriteRendererComponent, TransformComponent>();
			for (auto entity : view)
			{
				auto [transform, quadSprite] = view.get<TransformComponent, PixelSpriteRendererComponent>(entity);
				float tiling = 1.0f;
				Engine::VulkanRenderer2D::DrawTextureQuad(transform.GetTransform(), quadSprite.Texture, tiling, quadSprite.Color);
			}
		}
		{
			auto view = m_registry.view<SpriteRendererComponent, TransformComponent>();
			for (auto entity : view)
			{
				auto [transform, quadSprite] = view.get<TransformComponent, SpriteRendererComponent>(entity);
				float tiling = 1.0f;
				Engine::VulkanRenderer2D::DrawTextureQuad(transform.GetTransform(), quadSprite.Texture, tiling, quadSprite.Color);
			}
		}

        //Engine::Renderer::DrawFrame();
        Engine::VulkanRenderer2D::EndScene();
    }

    void Scene::OnViewportResize(uint32_t width, uint32_t height, glm::vec2 viewportBounds)
    {
        m_viewportHeight = height;
        m_viewportWidth = width;
		m_viewportBounds[0] = viewportBounds; // min only for now

        auto view = m_registry.view<CameraComponent>();

        for (auto entity : view)
        {
            auto cameraComp = view.get<CameraComponent>(entity);
            if (!cameraComp.FixedAspectRatio)
            {
                cameraComp.Camera.SetViewportSize(width, height);
                cameraComp.Camera.SetViewportBounds(viewportBounds);

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

    /*
    void Scene::UpdatePhysics(Timestep timestep)
    {
        EE_PROFILE_FUNCTION();

        constexpr int32_t subStepCount = 4;
        constexpr float physicsStep = 1.0f / 60.0f;  // Precomputed constant

        // Step physics world in parallel (with multithreading)
        b2World_Step(m_worldId, physicsStep, subStepCount);

        // Parallelize physics state updates
        size_t count = m_renderSOA.BodyIds.size();

        // Use a parallel loop to update transforms
        std::for_each(std::execution::par, m_renderSOA.BodyIds.begin(), m_renderSOA.BodyIds.end(),
            [&](b2BodyId& bodyId)
            {
                // Find the index of the current body
                size_t index = &bodyId - m_renderSOA.BodyIds.data();

                // Update physics state for the current body
                b2Vec2 position = b2Body_GetPosition(bodyId);
                b2Rot rotation = b2Body_GetRotation(bodyId);
               

                // Directly update transform (modify the transform in the same array)
                m_renderSOA.InstanceTransforms[index] =
                    glm::translate(glm::mat4(1.0f), { position.x, position.y, 0.0f }) *
                    glm::rotate(glm::mat4(1.0f), std::atan2(rotation.s, rotation.c), { 0, 0, 1 }) *
                    glm::scale(glm::mat4(1.0f), Math::extractScaleFromMat4(m_renderSOA.InstanceTransforms[index]));
            }
        );
    }
    */
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

    void Scene::RegisterSystem(const std::function<void(entt::registry&, float, Scene* scene)>& system)
    {
        m_gameplaySystems.emplace_back(system);
        EE_CORE_INFO("System registered");
    }

    template<typename T>
    inline void Scene::OnComponentAdded(Entity entity, T& component)
    {
        

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
    void Scene::OnComponentAdded<PixelSpriteRendererComponent>(Entity entity, PixelSpriteRendererComponent& component)
    {

    }
    
    template<>
    void Scene::OnComponentAdded<IDComponent>(Entity entity, IDComponent& component)
    {

    }

    template<>
    void Scene::OnComponentAdded<CharacterControllerComponent>(Entity entity, CharacterControllerComponent& component)
    {

    }

    template<>
    void Scene::OnComponentAdded<ProjectileComponent>(Entity entity, ProjectileComponent& component)
    {

    }
    template<>
    void Scene::OnComponentAdded<HealthComponent>(Entity entity, HealthComponent& component)
    {

    }

    template<>
    void Scene::OnComponentAdded<NPCAIMovementComponent>(Entity entity, NPCAIMovementComponent& component)
    {

    }

    template<>
    void Scene::OnComponentAdded<NPCAIVisionComponent>(Entity entity, NPCAIVisionComponent& component)
    {

    }

    template<>
    void Scene::OnComponentAdded<WeaponComponent>(Entity entity, WeaponComponent& component)
    {

    }
}
