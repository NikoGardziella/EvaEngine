#include "pch.h"
#include "Scene.h"
#include "Engine.h"

#include "Component.h"
#include "Engine/Scene/Components/Combat/HealthComponent.h"
#include "Engine/Scene/Components/Combat/WeaponComponent.h"
#include <Engine/Scene/Components/Player/CharacterControllerComponent.h>
#include "Engine/AssetManager/AssetManager.h"

#include "glm/gtc/matrix_transform.hpp"
#include <glm/glm.hpp>
#include "box2d/box2d.h"
#include "box2d/math_functions.h"
#include "Components/NPC/NpcAIComponent.h"
#include "Engine/Debug/DebugInterface.h"
#include "Components/Render/TileComponent.h"
#include "Components/Render/ChunkRendererComponent.h"
#include "Components/Render/RoofRenderComponent.h"
#include "Components/Vehicles/VehicleComponent.h"
#include "Components/Vehicles/DriverComponent.h"
#include "Components/Projectiles/ProjectileComponent.h"


namespace Engine {

	uint32_t PROFILING = 0;

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
		EE_CORE_INFO("Creating new Scene");
        m_registry = entt::registry();
        m_gridMap = std::make_shared<GridMap>();
        m_textureStreamingSystem = std::make_unique<TextureStreamingSystem>();
        m_textureStreamingSystem->SetGridMap(m_gridMap);
        DebugInterface::SetTextureStreamingSystem(m_textureStreamingSystem.get());

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
		newScene->m_viewportBounds[1] = other->m_viewportBounds[1];

		newScene->SetTextureStreamingSystem(other->GetTextureStreamingSystemRef());

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
            
        }
    }




    Ref<Scene> Scene::Combine(Ref<Scene> sceneA, Ref<Scene> sceneB)
    {
        Ref<Scene> combinedScene = std::make_shared<Scene>();

        // usually sceneB is the Game scene that has the TextureStreaming
        combinedScene->SetTextureStreamingSystem(sceneB->GetTextureStreamingSystemRef());



        combinedScene->m_viewportWidth = sceneA->m_viewportWidth;
        combinedScene->m_viewportHeight = sceneA->m_viewportHeight;
        combinedScene->m_viewportBounds[0] = sceneA->m_viewportBounds[0];
        combinedScene->m_viewportBounds[1] = sceneA->m_viewportBounds[1];

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
        CopyComponent<TileComponent>(dstSceneRegistry, srcSceneRegistry, enttMap);
        CopyComponent<RoofRenderComponent>(dstSceneRegistry, srcSceneRegistry, enttMap);
        CopyComponent<VehicleComponent>(dstSceneRegistry, srcSceneRegistry, enttMap);
        CopyComponent<DriverComponent>(dstSceneRegistry, srcSceneRegistry, enttMap);
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



        DebugInterface::SetTextureStreamingSystem(m_textureStreamingSystem.get());

        // makes sure textures are reloaded to the right registry
        // editor to game
        m_textureStreamingSystem->ResetAllChunks(m_registry);
        m_textureStreamingSystem->BakeTilesIntoChunks(m_registry);
		m_textureStreamingSystem->AddChunkEntitiesToRegistry(m_registry);

       // m_gridMap->BuildFromRegistry(m_registry);
    }





    void Scene::OnRunTimeStop()
    {
        b2DestroyWorld(m_worldId);

        m_gridMap->Clear();
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
        ║  EVA ENGINE | ENTT                             ║
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

        // get player info
        auto playerView = m_registry.view<Engine::TransformComponent, CharacterControllerComponent, Engine::CircleCollider2DComponent, Engine::IDComponent>();
        glm::vec2 playerPos;
        uint64_t playerID = 0;

        Entity playerEntity = Entity{};
        for (auto entity : playerView)
        {
            auto& playerTransform = playerView.get<Engine::TransformComponent>(entity);
            playerPos.x = playerTransform.Translation.x;
            playerPos.y = playerTransform.Translation.y;

            auto& playerIDComp = playerView.get<Engine::IDComponent>(entity);
            playerID = playerIDComp.ID;
            playerEntity = Entity{ entity, this };
        }



        //************ update scripts *************** // Remove?
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
            // Remove Box 2d physics
            UpdatePhysics(timestep);
        }


        Camera* mainCamera = nullptr;
        CameraComponent& mainCameraComp = CameraComponent{};
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
						mainCameraComp = camera;
                        break;
                    }
                }
            }
        }

        if(mainCamera)
        {   

            //Renderer2D::BeginScene(mainCamera->GetViewProjection(), cameraTransform);
            Engine::VulkanRenderer2D::BeginScene(mainCamera->GetViewProjection(), cameraTransform);


            //*********** GPU COLLISIONS & RENDER ***********
            {
                {
                    EE_PROFILE_SCOPE("chunk render");

                    entt::basic_view view = m_registry.view<ChunkRendererComponent, TransformComponent>();
                    glm::vec4 cameraBounds = mainCameraComp.Camera.CalculateCameraWorldBounds(mainCameraComp.Camera, cameraTransform);
                    glm::vec2 camMin = glm::vec2(cameraBounds.x, cameraBounds.y);
                    glm::vec2 camMax = camMin + glm::vec2(cameraBounds.z, cameraBounds.w);
                    for (auto entity : view)
                    {
                        auto [transform, chunkComp] = view.get<TransformComponent, ChunkRendererComponent>(entity);
                        {
                            if (!chunkComp.IsLoaded)
                                continue;

                            float tiling = 1.0f;
                            glm::vec4 color = glm::vec4(1);

                           
                            glm::vec2 worldPos = glm::vec2(chunkComp.ChunkCoords) * (float)CHUNK_SIZE + glm::vec2(CHUNK_SIZE * 0.5f);
                            glm::mat4 model =
                                glm::translate(glm::mat4(1.0f),
                                    glm::vec3(worldPos.x, worldPos.y, 0.0f))
                                * glm::scale(glm::mat4(1.0f),
                                    glm::vec3(CHUNK_SIZE, CHUNK_SIZE, 1.0f));
                           
                            float pixelSize = (float)CHUNK_SIZE / chunkComp.Texture->GetWidth();;
                            glm::vec2 textureOrigin;
                            textureOrigin.x = worldPos.x - CHUNK_SIZE * 0.5f;
                            textureOrigin.y = worldPos.y - CHUNK_SIZE * 0.5f;
                            
                            chunkComp.Texture->SetCheckCollision(true);
                            chunkComp.Texture->SetTextureOrigin(textureOrigin);
                            chunkComp.Texture->SetPixelSize(pixelSize);

                            Engine::VulkanRenderer2D::DrawTextureQuadWithHealth(model, chunkComp.Texture, chunkComp.HealthTexture);
                        }
                    }
                }

                {
                    EE_PROFILE_SCOPE("roof render");


                    entt::basic_view view = m_registry.view<RoofRenderComponent, TransformComponent>();
                    for (auto entity : view)
                    {
                        auto [transform, roofSprite] = view.get<TransformComponent, RoofRenderComponent>(entity);
                        if (!roofSprite.IsLoaded)
                            continue;


                        glm::vec2 textureSizeInTiles = glm::vec2(
                            roofSprite.Texture->GetWidth(),
                            roofSprite.Texture->GetHeight()
                        ) / float(PIXELS_IN_TILE);

                        glm::vec2 textureSizeWorld = textureSizeInTiles * float(TILE_SIZE);

                        float tiling = 1.0f;
                        glm::vec4 color = glm::vec4(1);
                        roofSprite.Texture->SetCheckCollision(false);

                        glm::mat4 model =
                            glm::translate(glm::mat4(1.0f), transform.Translation + roofSprite.LocalOffset) *
                            glm::scale(glm::mat4(1.0f), glm::vec3(textureSizeWorld, 1.0f));


                        // World origin = entity position + local offset
                        glm::vec2 roofMin = glm::vec2(transform.Translation) + glm::vec2(roofSprite.LocalOffset);
                        glm::vec2 roofMax = roofMin + textureSizeWorld;

                        glm::vec2 playerCenter = playerPos + glm::vec2(1.1f, 1.1f);
                        glm::ivec2 tileCoord = glm::floor(playerPos / float(TILE_SIZE));

                        float margin = 0.1f; 

                        glm::vec2 expandedMin = roofMin - glm::vec2(margin);
                        glm::vec2 expandedMax = roofMax + glm::vec2(margin);

                        glm::bvec2 minCheck = glm::greaterThanEqual(playerCenter, expandedMin);
                        glm::bvec2 maxCheck = glm::lessThanEqual(playerCenter, expandedMax);
                        if (glm::all(minCheck) && glm::all(maxCheck))
                        {
                            continue;
                        }
                       
                        Engine::VulkanRenderer2D::DrawTextureQuad(model, roofSprite.Texture, tiling, color);
                        //Engine::VulkanRenderer2D::DrawLineRect(model, glm::vec4(1, 0, 0, 0.3f), -1);
                    }

                }

                {
                    EE_PROFILE_SCOPE("Texture update");

                    entt::basic_view view = m_registry.view<SpriteRendererComponent, TransformComponent, IDComponent>();
                    glm::vec4 cameraBounds = mainCameraComp.Camera.CalculateCameraWorldBounds(mainCameraComp.Camera, cameraTransform);
                    glm::vec2 camMin = glm::vec2(cameraBounds.x, cameraBounds.y);
                    glm::vec2 camMax = camMin + glm::vec2(cameraBounds.z, cameraBounds.w);
                    for (auto entity : view)
                    {
                        auto [transform, quadSprite, IDcomp] = view.get<TransformComponent, SpriteRendererComponent, IDComponent>(entity);
                        {
                            {
                               // EE_PROFILE_SCOPE("Texture culling");

                                {

                                 //   EE_PROFILE_SCOPE("Remove projectiles");
                                    if (m_registry.any_of<ProjectileComponent>(entity))
                                        continue; // skip entities with ProjectileComponent
                                }
                                {
                                   // EE_PROFILE_SCOPE("Remove empty textures");
                                    if (!quadSprite.Texture)
                                    {
                                        continue;
                                    }

                                }

                                {
                                    glm::mat4 view = glm::inverse(cameraTransform);
                                    glm::mat4 proj = mainCamera->GetViewProjection();
                                    
                                    glm::vec2 entityPos = glm::vec2(transform.Translation.x, transform.Translation.y);
                                    glm::vec2 entityHalfSize = glm::vec2(transform.Scale.x, transform.Scale.y) * 0.5f;

                                    glm::vec2 entityMin = entityPos - entityHalfSize;
                                    glm::vec2 entityMax = entityPos + entityHalfSize;

                                    // Fast AABB vs AABB test
                                    bool visible =
                                        entityMax.x >= camMin.x && entityMin.x <= camMax.x &&
                                        entityMax.y >= camMin.y && entityMin.y <= camMax.y;

                                    if (!visible)
                                        continue;
                                }
                            }
                        }

                        {
                            //EE_PROFILE_SCOPE("DrawTextureQuad");

                            float tiling = 1.0f;
                            if (m_registry.any_of<CharacterControllerComponent>(entity) &&
                                !m_registry.any_of<DriverComponent>(entity))
                            {
                                // only render player from here for now
                                // if player is NOT in vehicle
                                
                                Engine::VulkanRenderer2D::DrawTextureQuad(transform.GetTransform(), quadSprite.Texture, tiling, quadSprite.Color);

                            }

                            
                            if (m_registry.any_of<VehicleComponent>(entity))
                            {
                                VehicleComponent& vehicleComp = m_registry.get<VehicleComponent>(entity);

                                glm::vec2 textureSizeInTiles = glm::vec2(
                                    quadSprite.Texture->GetWidth(),
                                    quadSprite.Texture->GetHeight()
                                ) / float(PIXELS_IN_TILE);

                                glm::vec2 textureSizeWorld = textureSizeInTiles * float(TILE_SIZE);

                                float tiling = 1.0f;
                                glm::vec4 color = glm::vec4(1);

                                glm::mat4 model =
                                    glm::translate(glm::mat4(1.0f), transform.Translation) *
                                    glm::rotate(glm::mat4(1.0f), transform.Rotation.z + 0.0f , glm::vec3(0.0f, 0.0f, 1.0f)) *
                                    glm::scale(glm::mat4(1.0f), glm::vec3(textureSizeWorld, 1.0f));
                                
                                float pixelSize = quadSprite.Texture->GetWidth();
                              
                                quadSprite.Texture->SetCheckCollision(false);
                                //quadSprite.Texture->SetTextureOrigin(transform.Translation);
                               // quadSprite.Texture->SetPixelSize(pixelSize);
                                float radius = 0.1f;

                               // glm::vec2 size = glm::vec2{ quadSprite.Texture->GetWidth()  ,quadSprite.Texture->GetHeight() };
                                glm::vec2 size = glm::vec2{ 2  ,1 };

                                uint32_t vehicleCurrentSpeed = (uint32_t)vehicleComp.CurrentSpeed;
                                uint32_t vehicleMass = (uint32_t)vehicleComp.Mass;
                                uint32_t decreaseForceMultiplier = 500;
                                uint32_t vehicleCollisionDamge = (vehicleMass * vehicleCurrentSpeed * vehicleCurrentSpeed) / decreaseForceMultiplier;
                                
                                Engine::VulkanRenderer2D::CalculateBoxCollision(transform.Translation, size, transform.Rotation.z, IDcomp.ID, eCollisionType::VEHICLE, vehicleCollisionDamge);

                                Engine::VulkanRenderer2D::DrawTextureQuad(model, quadSprite.Texture, tiling, quadSprite.Color);

                            }
                        }

                        
                        {
                          //  EE_PROFILE_SCOPE("Set texture stuff");


                            if (m_registry.any_of<CharacterControllerComponent>(entity))
                                continue; // skip entities with CharacterControllerComponent
                            float pixelSize = transform.Scale.x / quadSprite.Texture->GetWidth();

                            glm::vec2 textureOrigin;
                            textureOrigin.x = transform.Translation.x - transform.Scale.x * 0.5f; // to bottom left

                            textureOrigin.y = transform.Translation.y - transform.Scale.y * 0.5f; // to bottom left
                           // quadSprite.Texture->SetCheckCollision(true);
                            quadSprite.Texture->SetTextureOrigin(textureOrigin);
                            quadSprite.Texture->SetPixelSize(pixelSize);

                        }
                        
                   
                    }

                }
                {
                    //EE_PROFILE_SCOPE("Projectiles and Player");

                    auto projectileView = m_registry.view<ProjectileComponent, TransformComponent, IDComponent, SpriteRendererComponent>();

                    float playerRadius = 0.5f;
                    float bulletRadius = 0.05f;
                    for (auto projectileEntity : projectileView)
                    {

                        auto [projectileTransform, projectile, IDComp, spriteComp] = projectileView.get<TransformComponent, ProjectileComponent, IDComponent, SpriteRendererComponent>(projectileEntity);
                        glm::vec2 projectilePos;
                        projectilePos.x = projectileTransform.Translation.x;
                        projectilePos.y = projectileTransform.Translation.y;

                      //  projectileTransform.Translation.z = 0.1f;
     
                        Engine::VulkanRenderer2D::CalculateCircleCollision(projectilePos, bulletRadius, IDComp.ID, eCollisionType::PROJECTILE, projectile.Damage);
                        Engine::VulkanRenderer2D::DrawProjectile(projectileTransform.GetTransform(), spriteComp.Texture, spriteComp.Color);

                    }
                    if (!playerEntity.HasComponent<DriverComponent>())
                    {
                        uint32_t plauerCollisionDamage = 0; // player does no damage on collision
                        Engine::VulkanRenderer2D::CalculateCircleCollision(playerPos, playerRadius, playerID, eCollisionType::PLAYER, plauerCollisionDamage);
                    }
                }

            }

            //*********** Render ************


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
                auto view = m_registry.view<SpriteRendererComponent, TransformComponent, VehicleComponent>();

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
                EE_PROFILE_SCOPE("Update Runtime SpriteRendererComponent");
                auto view = m_registry.view<SpriteRendererComponent, TransformComponent, NPCAIVisionComponent>();

                for (auto entity : view)
                {
                    auto [transform, quadSprite] = view.get<TransformComponent, SpriteRendererComponent>(entity);
                    // check what is this actually rendering,.
                    if (m_gridMap->HasLineOfSight(playerPos, transform.Translation, m_debugDrawLOS))
                    {
                        float tiling = 1.0f;
                        Engine::VulkanRenderer2D::DrawTextureQuad(transform.GetTransform(), quadSprite.Texture, tiling, quadSprite.Color);
                    }
                }

            }

            //Engine::Renderer::DrawFrame();
            Engine::VulkanRenderer2D::EndScene();


        }
        m_textureStreamingSystem->Update(playerPos, m_registry);

    }

    void Scene::OnUpdateECSRuntime(Timestep timestep)
    {
        EE_PROFILE_FUNCTION();

        //********** Update all systems **************
        {
            for (auto& system : m_gameplaySystems)
            {
                system(m_registry, timestep, this);
            }
        }

    }


    void Scene::OnUpdateEditor(Timestep timestep, EditorCamera& camera)
    {
        EE_PROFILE_FUNCTION();

        Engine::VulkanRenderer2D::BeginScene(camera);


        {

            auto playerView = m_registry.view<Engine::TransformComponent, CharacterControllerComponent, Engine::CircleCollider2DComponent, Engine::IDComponent>();
            glm::vec2 playerPos;

            for (auto playerEntity : playerView)
            {
                auto& playerTransform = playerView.get<Engine::TransformComponent>(playerEntity);
                playerPos.x = playerTransform.Translation.x;
                playerPos.y = playerTransform.Translation.y;

            }

            m_textureStreamingSystem->Update(playerPos, m_registry);
        }


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
                if (quadSprite.Texture == nullptr)
                {
                    continue;
                }

				Engine::VulkanRenderer2D::DrawTextureQuad(transform.GetTransform(), quadSprite.Texture, tiling, quadSprite.Color);
			}
		}
        {
            auto view = m_registry.view<TileComponent, TransformComponent>();
            for (auto entity : view)
            {
                auto [transform, quadSprite] = view.get<TransformComponent, TileComponent>(entity);
                float tiling = 1.0f;
                if (quadSprite.Texture == nullptr)
                {
                    continue;
                }
				glm::vec4 color = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
                Engine::VulkanRenderer2D::DrawTextureQuad(transform.GetTransform(), quadSprite.Texture, tiling, color);
            }
        }
        {
            
            auto view = m_registry.view<TileComponent, TransformComponent>();
            for (auto entity : view)
            {
                glm::vec4 color = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);

                TileComponent& tileComponent = view.get<TileComponent>(entity);
                TransformComponent& transformComponent = view.get<TransformComponent>(entity);
                for (size_t i = 0; i < tileComponent.tiles.size(); i++)
                {
                    float flippedV0 = tileComponent.tiles[i].UV.w; // original v1 (bottom)
                    float flippedV1 = tileComponent.tiles[i].UV.y; // original v0 (top)
                    glm::vec4 flippedUV = glm::vec4(tileComponent.tiles[i].UV.x, flippedV0, tileComponent.tiles[i].UV.z, flippedV1);


                    glm::vec2 worldPos = glm::vec2(transformComponent.Translation) + tileComponent.tiles[i].position;

                    // Use flippedUV for rendering, don't overwrite original UV
                    Engine::VulkanRenderer2D::DrawTile(worldPos, flippedUV, color);
                }
                
            }
            
        }
        //Engine::Renderer::DrawFrame();
        Engine::VulkanRenderer2D::EndScene();
    }

    void Scene::OnViewportResize(uint32_t width, uint32_t height, std::array<glm::vec2, 2> viewportBounds)
    {
        m_viewportHeight = height;
        m_viewportWidth = width;
		m_viewportBounds[0] = viewportBounds[0]; 
		m_viewportBounds[1] = viewportBounds[1]; 

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
        //CopyComponentIfExists<SpriteRendererComponent>(newEntity, entity);
        CopyComponentIfExists<CameraComponent>(newEntity, entity);
        CopyComponentIfExists<BoxCollider2DComponent>(newEntity, entity);
        CopyComponentIfExists<RigidBody2DComponent>(newEntity, entity);
        CopyComponentIfExists<NativeScriptComponent>(newEntity, entity);
        CopyComponentIfExists<CircleRendererComponent>(newEntity, entity);
        CopyComponentIfExists<CircleCollider2DComponent>(newEntity, entity);
        CopyComponentIfExists<TileComponent>(newEntity, entity);

        if (entity.HasComponent<SpriteRendererComponent>())
        {
            SpriteRendererComponent& spriteComp =  newEntity.AddComponent<SpriteRendererComponent>();
            //spriteComp.Texture = AssetManager::CloneTexture(entity.GetComponent<SpriteRendererComponent>().Texture->GetName());
            std::vector<uint8_t> pixelData;
            std::vector<uint8_t> healthData;
            int width, height;

            if (AssetManager::GetTexturePixelData(entity.GetComponent<SpriteRendererComponent>().Texture->GetName(), pixelData, healthData, width, height))
            {
                EE_CORE_WARN("health data not implemented to GetTexturePixelData");
                
				m_textureStreamingSystem->UploadToChunkFromTexture(
					entity.GetComponent<TransformComponent>().Translation,
					entity.GetComponent<IDComponent>().ID,
                    entity.GetComponent<SpriteRendererComponent>().Texture->GetName(),
					pixelData, healthData, width, height);
                
            }
        }

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

    void Scene::RegisterSystem(const std::function<void(entt::registry&, float, Scene* scene)>& system)
    {
        m_gameplaySystems.emplace_back(system);
        EE_CORE_INFO("System registered");
    }

    /*
   
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

     */
}
