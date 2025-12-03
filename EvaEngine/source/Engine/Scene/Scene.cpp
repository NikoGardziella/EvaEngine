#include "pch.h"
#include "Scene.h"
#include "Engine.h"

#include "Engine/Scene/Components/Combat/HealthComponent.h"
#include "Engine/Scene/Components/Combat/WeaponComponent.h"
#include <Engine/Scene/Components/Player/CharacterControllerComponent.h>
#include "Engine/AssetManager/AssetManager.h"
#include <Engine/Animation/3D/System/CullingSystem3D.h>
#include <Engine/Animation/3D/System/RenderSystem3D.h>
#include "Engine/Animation/3D/VisibleSet.h"
#include "Engine/Map/Grid/GridMap.h"
#include <Engine/Core/UUID.h>
#include "Engine/Debug/DebugInterface.h"
#include "Engine/Map/Tile/TileManager.h"
#include <Engine/Animation/2D/AnimationSystem2D.h>

#include "Component.h"
#include "Components/Render/TileComponent.h"
#include "Components/Render/ChunkRendererComponent.h"
#include "Components/Render/RoofRenderComponent.h"
#include "Components/Vehicles/VehicleComponent.h"
#include "Components/Vehicles/DriverComponent.h"
#include "Components/Projectiles/ProjectileComponent.h"
#include "Components/Render/DynamicObjectRenderComp.h"
#include "Components/NPC/NpcAIComponent.h"
#include "Components/Animation/AnimationComponent.h"
#include "Components/Render/3D/MeshRefComponent.h"

#include "glm/gtc/matrix_transform.hpp"
#include <glm/glm.hpp>
#include "box2d/box2d.h"
#include "box2d/math_functions.h"
#include "Components/Render/3D/RenderBoundsComponent.h"
#include "Components/Render/3D/SkeletonComponent.h"
#include "Components/Render/3D/AnimatorComponent.h"

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
        m_tileMananger = std::make_unique<TileManager>();
        m_animationBank = std::make_unique<AnimationBank2D>();
        m_animationSystem = std::make_unique<AnimationSystem2D>(*m_animationBank);

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


    static void SpawnMeshGrid(Engine::Scene* scene, uint32_t meshId = 0,  uint32_t count = 100,
        uint32_t perRow = 10, float spacing = 2.0f, const glm::vec3& origin = glm::vec3(0.0f))
    {
        const MeshAsset& meshAsset = AssetManager::GetMeshFromMeshRegistry(meshId);
        const uint32_t submeshCount = (uint32_t)meshAsset.submeshes.size();

        for (uint32_t i = 0; i < count; ++i)
        {
            /*
            if (i % 2 == 0)
            {
                meshId = 1;
            }
            else
            {
                meshId = 0;
            }

            */
            // Grid coords
            const uint32_t r = i / perRow;
            const uint32_t c = i % perRow;

            // World position
            glm::vec3 pos = origin + glm::vec3((float)c * spacing, (float)r * spacing,0.0f );

            // Create entity and components
            Engine::Entity entity = scene->CreateEntity();

            auto& meshComp = entity.AddComponent<MeshRefComponent>();
            meshComp.meshId = meshId;
            meshComp.submeshFirst = 0;
            meshComp.submeshCount = submeshCount;

            auto& tr = entity.AddComponent<TransformComponent>();
            tr.Translation = pos;                // adjust field names if yours differ (e.g., translation/rotation/scale)
            tr.Rotation = glm::vec3(0.0f, 0.0f, 0.0f); // identity
            tr.Scale = glm::vec3(1.0f);

            RenderBoundsComponent& renderBoundsComp = entity.AddComponent<RenderBoundsComponent>();
            renderBoundsComp.maxL = meshAsset.maxL;
            renderBoundsComp.minL = meshAsset.minL;

            uint32_t skeletonId = 0;
            auto& skel = entity.AddComponent<SkeletonComponent>();
            skel.skeletonId = skeletonId;      // returned by importer
            skel.boneCount = AssetManager::GetSkeletonRegistry().Get(skeletonId).parent.size();
            skel.boneBase = 0xFFFFFFFFu;     // let BonePalette allocate


            uint32_t testClip = 1;
            uint32_t testClipB = 0;
            // Attach animator
            auto& anim = entity.AddComponent<AnimatorComponent>();
            anim.clipA = testClip;           
            anim.clipB = testClipB;
            anim.timeA = 0.0f;
            anim.blend = 0.0f;                 // only clipA
            anim.playbackSpeed = 1.0f;
            
        }
    }


    void Scene::OnRunTimeStart()
    {
     



        DebugInterface::SetTextureStreamingSystem(m_textureStreamingSystem.get());

        // makes sure textures are reloaded to the right registry
        // editor to game
        //m_textureStreamingSystem->ResetAllChunks(this);
        m_textureStreamingSystem->SortIsoTilesByY(this);

        m_tileMananger->BuildTemplatesForScene(this);
        m_tileMananger->BuildInitialResidency(this);
        m_gridMap->BuildFromRegistry(this);


        m_textureStreamingSystem->BakeTilesIntoChunks(this); // terrain
		m_textureStreamingSystem->AddChunkEntitiesToRegistry(this); 

       // m_gridMap->BuildFromRegistry(m_registry);


        m_cullingSystem3D = std::make_shared<CullingSystem3D>();
        m_transformSystem3D = std::make_shared<TransformSystem3D>();

        MeshAsset& meshAsset = AssetManager::GetMeshFromMeshRegistry(0);
        

        Entity entity3D = this->CreateEntity();
  

        SpawnMeshGrid(this,0, 10,10, 2);
    }





    void Scene::OnRunTimeStop()
    {

        m_textureStreamingSystem->UnloadAllChunks(this);
    }

    void Scene::PauseRuntime()
    {

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

       

       // Camera* mainCamera = nullptr;
        CameraComponent& mainCameraComp = CameraComponent{};
        glm::mat4 cameraTransform;
        glm::mat4 cameraView;
        {
            EE_PROFILE_SCOPE("Get Update Runtime Camera");

            {
                auto group = m_registry.group<TransformComponent, CameraComponent>();
                for (auto entity : group)
                {
                    auto [transform, camera] = group.get<TransformComponent, CameraComponent>(entity);

                    if (camera.Primary)
                    {
                        //mainCamera = &camera.Camera;
                        cameraTransform = transform.GetTransform();
						mainCameraComp = camera;
                        cameraView = glm::inverse(cameraTransform);
                        break;
                    }
                }
            }
        }
        glm::vec2 playerPos;
        auto playerView = m_registry.view<Engine::TransformComponent, CharacterControllerComponent, Engine::CircleCollider2DComponent, Engine::IDComponent, SpriteRendererComponent>();
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

            if (!playerEntity.HasComponent<DriverComponent>())
            {
                float playerRadius = 0.5f;
                float tiling = 0.5f;

                auto& spriteComp = playerView.get<Engine::SpriteRendererComponent>(entity);

                if (!playerEntity.HasComponent<AnimationComponent>())
                {
                    auto clipId = m_animationBank->Load2DClipFromYaml("animations/player/data/run.yaml");
                    auto& animComp = playerEntity.AddComponent<AnimationComponent>();
                    animComp.clipId = clipId;
                    animComp.dirMode = 1;

                   // auto& animStateComp = playerEntity.AddComponent<AnimatorStateComponent>();
                }


               // Engine::VulkanRenderer2D::DrawTextureQuad(playerTransform.GetTransform(), spriteComp.Texture, tiling, glm::vec4(1));
                Engine::VulkanRenderer2D::CalculatePlayerCircleCollision(playerPos, playerRadius, playerID, eCollisionType::PLAYER);
            }

            if (!playerEntity.HasComponent<AnimatorComponent>())
            {
                uint32_t meshId = 0;
                const MeshAsset& meshAsset = AssetManager::GetMeshFromMeshRegistry(meshId);
                const uint32_t submeshCount = (uint32_t)meshAsset.submeshes.size();


                auto& meshComp = playerEntity.AddComponent<MeshRefComponent>();
                meshComp.meshId = meshId;
                meshComp.submeshFirst = 0;
                meshComp.submeshCount = submeshCount;

             

                RenderBoundsComponent& renderBoundsComp = playerEntity.AddComponent<RenderBoundsComponent>();
                renderBoundsComp.maxL = meshAsset.maxL;
                renderBoundsComp.minL = meshAsset.minL;

                uint32_t skeletonId = 0;
                auto& skel = playerEntity.AddComponent<SkeletonComponent>();
                skel.skeletonId = skeletonId;      // returned by importer
                skel.boneCount = AssetManager::GetSkeletonRegistry().Get(skeletonId).parent.size();
                skel.boneBase = 0xFFFFFFFFu;     // let BonePalette allocate


                uint32_t testClip = 1;
                uint32_t testClipB = 0;
                // Attach animator
                auto& anim = playerEntity.AddComponent<AnimatorComponent>();
                anim.clipA = testClip;
                anim.clipB = testClipB;
                anim.timeA = 0.0f;
                anim.blend = 0.0f;                 // only clipA
                anim.playbackSpeed = 1.0f;
            }
            


        }
        m_textureStreamingSystem->Update(playerPos, this);

        m_animationSystem->Update(timestep, this);

        m_transformSystem3D->Update(this, timestep);

        m_animationSystem3D.Update(this, timestep, AssetManager::GetSkeletonRegistry(), AssetManager::GetAnimationRegistry(), m_bonePaletteBuffer);

        //if(mainCameraComp.Camera != entt::null)
        {   


            VisibleSet& visibleSet = m_cullingSystem3D->BuildVisible(this, mainCameraComp.Camera, *m_transformSystem3D, cameraTransform);

            m_renderSystem3D.Render(visibleSet, this, *m_transformSystem3D,
                AssetManager::GetMeshRegistry(), AssetManager::GetMaterialRegistry());


            //Renderer2D::BeginScene(mainCamera->GetViewProjection(), cameraTransform);
            Engine::VulkanRenderer2D::BeginScene(mainCameraComp.Camera.GetProjection(), cameraTransform);

            Engine::VulkanRenderer3D::Begin3DScene(mainCameraComp.Camera.GetProjection(),cameraView);

            glm::ivec2 minOrigin = { std::numeric_limits<int>::max(), std::numeric_limits<int>::max() };

            //*********** GPU COLLISIONS & RENDER ***********
            {
                // draw terrain after the 
                entt::basic_view view = m_registry.view<ChunkRendererComponent>();
                for (auto entity : view)
                {
                    ChunkRendererComponent& chunkComp = view.get<ChunkRendererComponent>(entity);
                    {
                        if (!chunkComp.IsLoaded)
                            continue;

                        if (chunkComp.TerrainTexture == nullptr)
                        {
                            continue;
                        }
                        

                        glm::vec2 worldPos = glm::vec2(chunkComp.ChunkCoords) * (float)CHUNK_SIZE + glm::vec2(CHUNK_SIZE * 0.5f);
                        chunkComp.TerrainTexture->SetTextureOrigin(worldPos);
                       // chunkComp.VisualEffectTexture->SetTextureOrigin(worldPos);

                        glm::mat4 model =
                            glm::translate(glm::mat4(1.0f),
                                glm::vec3(worldPos.x, worldPos.y, 0.0f))
                            * glm::scale(glm::mat4(1.0f),
                                glm::vec3(CHUNK_SIZE, CHUNK_SIZE, 1.0f));

                        

                        Engine::VulkanRenderer2D::DrawVisualEffectTexture(model, chunkComp.VisualEffectTexture);
                        Engine::VulkanRenderer2D::DrawTextureQuad(model, chunkComp.TerrainTexture);


                    }
                }


                
                this->ForEach<TransformComponent, DynamicObjectRenderComp>(
                    [&](Entity, TransformComponent& tr, DynamicObjectRenderComp& dyn)
                    {
                        if (!dyn.IsLoaded || !dyn.Texture || !dyn.PropertiesTexture) return;

                        const float pxWorld = float(TILE_SIZE) / float(TILE_PIXEL_WIDTH);

                        // Entity anchor is bottom-center. Our baked origin is bottom-left relative to the anchor.
                        glm::vec2 anchor = glm::vec2(tr.Translation);        // your bottom-center ground anchor
                        glm::vec2 randomOffset = glm::vec2(0.5f, 1.0f);  // to bottom left tile 128 x 256
                        glm::vec2 originBL = anchor + dyn.OriginBLWorld + randomOffset;       // place bottom-left in world

                        // (optional) snap to pixel grid
                        originBL = glm::round(originBL / pxWorld) * pxWorld;

                        glm::mat4 model =
                            glm::translate(glm::mat4(1.0f), glm::vec3(originBL, 0.0f)) *
                            glm::scale(glm::mat4(1.0f), glm::vec3(dyn.WorldSize, 1.0f));

                        dyn.PropertiesTexture->SetTextureOrigin(originBL);
                        dyn.PropertiesTexture->SetPixelSize(pxWorld);
                        dyn.PropertiesTexture->SetCheckCollision(true);
                        dyn.Texture->SetTextureOrigin(originBL);
                        dyn.Texture->SetPixelSize(pxWorld);
                        dyn.Texture->SetCheckCollision(true);

                        //VulkanRenderer2D::DrawTextureQuadWithProperties(model, dyn.Texture, dyn.PropertiesTexture);
                    });



                {
                    /*
                    EE_PROFILE_SCOPE("chunk render");
                    glm::ivec2 minOrigin = glm::ivec2(std::numeric_limits<int>::max());

                    entt::basic_view view = m_registry.view<ChunkRendererComponent>();
                    glm::vec4 cameraBounds = mainCameraComp.Camera.CalculateCameraWorldBounds(mainCameraComp.Camera, cameraTransform);
                    glm::vec2 camMin = glm::vec2(cameraBounds.x, cameraBounds.y);
                    glm::vec2 camMax = camMin + glm::vec2(cameraBounds.z, cameraBounds.w);
                    for (auto entity : view)
                    {
                        ChunkRendererComponent& chunkComp = view.get<ChunkRendererComponent>(entity);
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
                           
                            float pixelSize = (float)CHUNK_SIZE / chunkComp.Texture->GetWidth();
                

                            glm::vec2 textureOrigin;
                            textureOrigin.x = worldPos.x - CHUNK_SIZE * 0.5f;
                            textureOrigin.y = worldPos.y - CHUNK_SIZE * 0.5f;
                            
                            chunkComp.PropertiesTexture->SetCheckCollision(true);
                            chunkComp.PropertiesTexture->SetTextureOrigin(textureOrigin);
                            chunkComp.PropertiesTexture->SetPixelSize(pixelSize);
                            chunkComp.Texture->SetCheckCollision(true);
                            chunkComp.Texture->SetTextureOrigin(textureOrigin);
                            chunkComp.Texture->SetPixelSize(pixelSize);


                            minOrigin.x = std::min(minOrigin.x, chunkComp.ChunkCoords.x);
                            minOrigin.y = std::min(minOrigin.y, chunkComp.ChunkCoords.y);

                            //EE_CORE_INFO("minOrigin {}, {}", minOrigin.x, minOrigin.y);
                          // Engine::VulkanRenderer2D::DrawTextureQuadWithProperties(model, chunkComp.Texture, chunkComp.PropertiesTexture);

                            
                        }
                    }
                    */
                    {
                        EE_PROFILE_SCOPE("tile render");
                        glm::vec2 minWorld = { std::numeric_limits<float>::infinity(),
                          std::numeric_limits<float>::infinity() };

                        // Minimal: uses your Scene::ForEachConst helper
                        ForEachConst<TransformComponent, TileComponent>(
                            [&](Entity e, const TransformComponent& tr, const TileComponent& tc)
                            {
                                for (const TileInfo& t : tc.tiles)
                                {
                                    if (t.Category == eTileCategory::Terrain)
                                        continue; // skip terrain


                                    // Trivial submit: NO residency work here, just append an instance
                                    VulkanRenderer2D::SubmitDestructibleTile(
                                        tr.Translation,   // entity world origin
                                        t.position,       // tile local offset
                                        t.UV,
                                        t.UID,            // precomputed UID  slot resolved elsewhere
                                        0.01f             // zBias
                                    );
                                    minWorld.x = std::min(minWorld.x, tr.Translation.x);
                                    minWorld.y = std::min(minWorld.y, tr.Translation.y);
                                }
                            });

                        ///EE_CORE_INFO("tile count: {}", tileCount);

                    }

                    //m_tileMananger->StreamInitialResidency(this);
                    //glm::ivec2 chunkMinOrigin = glm::floor(glm::vec2(minOrigin) / float(CHUNK_SIZE));
                    //glm::ivec2 tileMinOrigin = chunkMinOrigin * int(CHUNK_SIZE);

                }

                    m_gridMap->UpdateTiles();
                    m_destructibleTileSystem.OnTilesUpdated(this);
              
            
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
                        ) / float(TILE_PIXEL_WIDTH);

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
                    EE_PROFILE_SCOPE("Texture update"); // RMEOVE=

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
                                    glm::mat4 proj = mainCameraComp.Camera.GetProjection();
                                    
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


                            
                            if (m_registry.any_of<VehicleComponent>(entity))
                            {
                                VehicleComponent& vehicleComp = m_registry.get<VehicleComponent>(entity);

                                glm::vec2 textureSizeInTiles = glm::vec2(
                                    quadSprite.Texture->GetWidth(),
                                    quadSprite.Texture->GetHeight()
                                ) / float(TILE_PIXEL_WIDTH);

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
                    //EE_PROFILE_SCOPE("Projectiles");

                    auto projectileView = m_registry.view<ProjectileComponent, TransformComponent, IDComponent, SpriteRendererComponent>();

                    for (auto projectileEntity : projectileView)
                    {

                        auto [projectileTransform, projectile, IDComp, spriteComp] = projectileView.get<TransformComponent, ProjectileComponent, IDComponent, SpriteRendererComponent>(projectileEntity);
                        glm::vec2 projectilePos;
                        projectilePos.x = projectileTransform.Translation.x;
                        projectilePos.y = projectileTransform.Translation.y;
     
                        // make struct
                        Engine::VulkanRenderer2D::CalculateCircleCollision(projectilePos, projectile.ProjectileRadius, IDComp.ID,
                            eCollisionType::PROJECTILE, projectile.Damage, projectile.DestructionRadius, projectile.Direction,
                            projectile.TargetPositionAtFireTime, projectile.DistanceToTargetatFireTime, projectile.TargetPositionHeightZ1);

                        Engine::VulkanRenderer2D::DrawProjectile(projectileTransform.GetTransform(), spriteComp.Texture, spriteComp.Color);

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


            {
                // render visual effects last on top of everything else
                EE_PROFILE_SCOPE("Update visual effects");

                entt::basic_view view = m_registry.view<ChunkRendererComponent>();
                for (auto entity : view)
                {
                    ChunkRendererComponent& chunkComp = view.get<ChunkRendererComponent>(entity);
                    {
                        if (!chunkComp.IsLoaded)
                            continue;

                        if (chunkComp.TerrainTexture == nullptr)
                        {
                            continue;
                        }

                        glm::vec2 worldPos = glm::vec2(chunkComp.ChunkCoords) * (float)CHUNK_SIZE + glm::vec2(CHUNK_SIZE * 0.5f);
                        
                        chunkComp.VisualEffectTexture->SetTextureOrigin(worldPos);
                        
                        glm::mat4 model =
                            glm::translate(glm::mat4(1.0f),
                                glm::vec3(worldPos.x, worldPos.y, 0.0f))
                            * glm::scale(glm::mat4(1.0f),
                                glm::vec3(CHUNK_SIZE, CHUNK_SIZE, 1.0f));



                        Engine::VulkanRenderer2D::DrawTextureQuad(model, chunkComp.VisualEffectTexture);
                        //Engine::VulkanRenderer2D::DrawVisualEffectTexture(model, chunkComp.VisualEffectTexture);

                    }
                }

            }


            //Engine::Renderer::DrawFrame();
            Engine::VulkanRenderer2D::EndScene();


        }

    }

    void Scene::OnUpdateECSRuntime(Timestep timestep)
    {
        EE_PROFILE_FUNCTION();

        //********** Update all systems **************
        {
            for (auto& system : m_gameplaySystems)
            {
                system(timestep, this);
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

           // m_textureStreamingSystem->Update(playerPos, this);
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
                //Engine::VulkanRenderer2D::DrawTextureQuad(transform.GetTransform(), quadSprite.Texture, tiling, color);
            }
        }

        {
            
            auto viewTerrain = m_registry.view<TileComponent, TransformComponent>();
            const float step = float(TILE_SIZE);
            viewTerrain.use<TransformComponent>();

            for (auto entity : viewTerrain)
            {
                auto& tileComp = viewTerrain.get<TileComponent>(entity);
                auto& tr = viewTerrain.get<TransformComponent>(entity);

                for (const auto& t : tileComp.tiles)
                {
                    if (t.Category != eTileCategory::Terrain) continue;

                    // t.position is a WORLD delta (not iso). Do NOT round/convert it.
                    glm::vec2 worldPosCenter = glm::vec2(tr.Translation) + t.position;

                    // bottom tip (ground contact) for bottom-center pivot

                    // Flip V like before
                    glm::vec4 uv = t.UV;
                    glm::vec4 flippedUV(uv.x, uv.w, uv.z, uv.y);

                    Engine::VulkanRenderer2D::DrawTile(worldPosCenter, flippedUV, glm::vec4(1.0f));
                }
            }





            Engine::VulkanRenderer2D::EndScene();

            Engine::VulkanRenderer2D::BeginScene(camera);

            auto view = m_registry.view<TileComponent, TransformComponent>();
            view.use<TransformComponent>(); // this ensured the draw order!
            for (auto entity : view)
            {

                TileComponent& tileComponent = view.get<TileComponent>(entity);
                TransformComponent& transformComponent = view.get<TransformComponent>(entity);

                const float step = float(TILE_SIZE);
                for (size_t i = 0; i < tileComponent.tiles.size(); i++)
                {
                    if (tileComponent.tiles[i].Category == eTileCategory::Terrain)
                    {
                        // skip terrain and draw everything else 
                        continue;
                    }
                    glm::vec2 ground = glm::vec2(transformComponent.Translation) + tileComponent.tiles[i].position; // position = WORLD delta to GROUND
                   
                    // Flip V as before
                    glm::vec4 uv = tileComponent.tiles[i].UV;
                    glm::vec4 flippedUV(uv.x, uv.w, uv.z, uv.y);

                    glm::vec4 color = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
                    // Use flippedUV for rendering, don't overwrite original UV
                    Engine::VulkanRenderer2D::DrawTile(ground, flippedUV, color);
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

  

    void Scene::RegisterSystem(const std::function<void(float, Scene* scene)>& system)
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
