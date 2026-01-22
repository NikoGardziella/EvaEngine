#include "pch.h"

#include <Engine/Renderer/Utils/PlayerData.h>

#include "Engine/AssetManager/AssetManager.h"

#include <Engine/Animation/3D/System/TransformSystem3D.h>
#include <Engine/Animation/2D/AnimationSystem2D.h>
#include <Engine/Animation/3D/System/CullingSystem3D.h>

#include "Engine/Animation/3D/VisibleSet.h"

#include "Components/Player/CharacterControllerComponent.h"
#include "Components/Player/PlayerVisionComponent.h"
#include "Components/Vehicles/DriverComponent.h"
#include "Components/Render/3D/AnimatorComponent.h"
#include "Components/Render/3D/MeshRefComponent.h"
#include "Components/Render/3D/RenderBoundsComponent.h"
#include "Components/Render/3D/SkeletonComponent.h"
#include "Components/Combat/EquippedWeaponComponent.h"
#include "Components/Animation/PlayerAnimation/CharacterAnimSetComponent.h"
#include "Components/Animation/PlayerAnimation/CharacterAnimStateComponent.h"
#include "Components/UI/HUDStateComponent.h"
#include "Components/Render/ChunkRendererComponent.h"
#include "Components/Render/DynamicObjectRenderComp.h"
#include "Components/Render/RoofRenderComponent.h"
#include "Components/Projectiles/ProjectileComponent.h"
#include "Components/Vehicles/VehicleComponent.h"
#include "Components/Combat/ThrowableComponent.h"
#include "Components/NPC/NpcAIComponent.h"

namespace Engine {


    void Scene::OnUpdateRuntime(Timestep timestep, bool isPlaying)
    {

        EE_PROFILE_FUNCTION();
        /*

        // Full-owning group: The registry owns and tightly packs both SpriteRendererComponent and TransformComponent
        auto group = m_registry.group<SpriteRendererComponent, TransformComponent>();
         Pros: Fastest iteration speed, best memory locality.
         Cons: Less flexibility, requires full ownership.

        // Partial-owning group: Owns SpriteRendererComponent but references TransformComponent without owning it
        auto group = m_registry.group<SpriteRendererComponent>(entt::get<TransformComponent>);
         Pros: Keeps SpriteRendererComponent tightly packed, while still accessing TransformComponent.
         Cons: TransformComponent is looked up dynamically, adding slight overhead.

        // Non-owning group: Does not own any components, just filters entities that have both components
        auto group = m_registry.group<>(entt::get<SpriteRendererComponent, TransformComponent>);
         Pros: No memory reordering, keeps components untouched.
         Cons: Slightly slower than owning groups because it doesn’t pack memory efficiently.


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



        CameraComponent* mainCameraComp = nullptr;
        glm::mat4 cameraTransform = glm::mat4(1.0f);
        glm::mat4 cameraView = glm::mat4(1.0f);
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
                        mainCameraComp = &camera;
                        cameraView = glm::inverse(cameraTransform);
                    }

                }
            }


        }





        glm::vec2 playerPos;
        auto playerView = m_registry.view<Engine::TransformComponent, CharacterControllerComponent, Engine::CircleCollider2DComponent, Engine::IDComponent, SpriteRendererComponent>();
        uint64_t playerID = 0;
        PlayerData playerStateData;

        Entity playerEntity = Entity{};
        for (auto entity : playerView)
        {

            playerEntity = Entity{ entity, this };

            auto& playerTransform = playerView.get<Engine::TransformComponent>(entity);
            playerPos.x = playerTransform.Translation.x;
            playerPos.y = playerTransform.Translation.y;
            glm::vec3 camerapos = glm::vec3(cameraTransform[3]);
            float x = camerapos.x;
            float y = camerapos.y;



            playerStateData.CameraPos = camerapos;
            playerStateData.PlayerPos = playerPos;
            if (PlayerVisionComp* visionCOmp = playerEntity.TryGetComponent<PlayerVisionComp>())
            {
                playerStateData.visionRadiusW = visionCOmp->visionDistanceW;
            }
            else
            {
                playerStateData.visionRadiusW = 1.0f;
            }

            VulkanRenderer2D::SubmitPlayerData(playerStateData);

            auto& playerIDComp = playerView.get<Engine::IDComponent>(entity);
            playerID = playerIDComp.ID;


            if (!playerEntity.HasComponent<DriverComponent>())
            {
                float playerRadius = 0.5f;
                float tiling = 0.5f;

                auto& spriteComp = playerView.get<Engine::SpriteRendererComponent>(entity);


                // player collision is made with grid

               // Engine::VulkanRenderer2D::DrawTextureQuad(playerTransform.GetTransform(), spriteComp.Texture, tiling, glm::vec4(1));
                Engine::VulkanRenderer2D::CalculatePlayerCircleCollision(playerPos, playerRadius, playerID, eCollisionType::PLAYER);
            }

            if (!playerEntity.HasComponent<Animator3DComponent>())
            {
                EE_CORE_WARN("move this stuff somewher");
                MeshRegistry& meshReg = AssetManager::GetMeshRegistry();

                const MeshAsset* meshAsset = meshReg.GetMeshByKey("playerMeshes");

                const uint32_t submeshCount = (uint32_t)meshAsset->submeshes.size();


                auto& meshComp = playerEntity.AddComponent<MeshRefComponent>();
                meshComp.meshId = meshAsset->id;
                meshComp.submeshFirst = 0;
                meshComp.submeshCount = submeshCount;



                RenderBoundsComponent& renderBoundsComp = playerEntity.AddComponent<RenderBoundsComponent>();
                renderBoundsComp.maxL = meshAsset->maxL;
                renderBoundsComp.minL = meshAsset->minL;

                auto& skel = playerEntity.AddComponent<SkeletonComponent>();
                skel.skeletonId = meshAsset->skeletonId;      // returned by importer
                skel.boneCount = AssetManager::GetSkeletonRegistry().Get(meshAsset->skeletonId).parent.size();
                skel.boneBase = 0xFFFFFFFFu;     // let BonePalette allocate


                uint32_t testClip = 1;
                uint32_t testClipB = 0;
                // Attach animator
                /*
                */
                Animator3DComponent& anim = playerEntity.AddComponent<Animator3DComponent>();
                anim.clipA = testClip;
                anim.clipB = INVALID_CLIP;
                anim.timeA = 0.0f;
                anim.blend = 0.0f;                 // only clipA
                anim.playbackSpeed = 1.0f;
                anim.loopAclip = true;
                anim.boneModel.resize(skel.boneCount);

                TransformComponent& playerTransformComp = playerEntity.GetComponent<TransformComponent>();
                playerTransformComp.Rotation.x += glm::radians(90.0f);

                WeaponInventoryComponent& weaponInventoryComp = playerEntity.AddComponent<WeaponInventoryComponent>();
                weaponInventoryComp.equipDirty = true;
                // weaponInventoryComp.equippedWeaponDefId = 1;


                Engine::AnimationRegistry& animReg = Engine::AssetManager::GetAnimationRegistry();

                CharacterAnimSetComponent& CharacterAnimSetComp = playerEntity.AddComponent<CharacterAnimSetComponent>();
                CharacterAnimSetComp.idle = animReg.FindAnimationClip("MaleIdleAnim")->id;
                CharacterAnimSetComp.run = animReg.FindAnimationClip("playerAnimRun")->id;
                CharacterAnimSetComp.aimIdle = animReg.FindAnimationClip("playerAnimAimRifle")->id;
                CharacterAnimSetComp.fireRifle = animReg.FindAnimationClip("playerAnimShootRifle")->id;



                CharacterAnimStateComponent& CharacterAnimStateComp = playerEntity.AddComponent<CharacterAnimStateComponent>();
                HUDStateComponent& HUDStateComp = playerEntity.AddComponent<HUDStateComponent>();
                AmmoComponent& AmmoComp = playerEntity.AddComponent<AmmoComponent>();
                PlayerVisionComp& visionComp = playerEntity.AddComponent<PlayerVisionComp>();
                visionComp.visionDistanceW = 10.0f;
                /// there will bne day i will move this
            }



        }

        m_textureStreamingSystem->Update(playerPos, this);

        m_animationSystem->Update(timestep, this);

        m_transformSystem3D->Update(this, timestep);

        m_animationSystem3D.Update(this, timestep, AssetManager::GetSkeletonRegistry(), AssetManager::GetAnimationRegistry());

        if (mainCameraComp != nullptr)
        {


            VisibleSet& visibleSet = m_cullingSystem3D->BuildVisible(this, mainCameraComp->Camera, *m_transformSystem3D, cameraTransform, m_fogOfWar);

            m_renderSystem3D.Render(visibleSet, this, *m_transformSystem3D,
                AssetManager::GetMeshRegistry(), AssetManager::GetMaterialRegistry());


            //Renderer2D::BeginScene(mainCamera->GetViewProjection(), cameraTransform);
            Engine::VulkanRenderer2D::BeginScene(mainCameraComp->Camera, cameraTransform);



            Engine::VulkanRenderer3D::Begin3DScene(mainCameraComp->Camera.GetProjection(), cameraView);


            m_fogOfWar->DrawFogOfWar(playerStateData, mainCameraComp->Camera, cameraTransform);

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
                        // chunkComp.TerrainTexture->SetTextureOrigin(worldPos); 
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
                //m_gridMap->HasLineOfSight(playerPos, glm::vec2(0.0f, 0.0f), true);
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
                    glm::vec4 cameraBounds = mainCameraComp->Camera.CalculateCameraWorldBounds(mainCameraComp->Camera, cameraTransform);
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
                                    glm::mat4 proj = mainCameraComp->Camera.GetProjection();

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
                                    glm::rotate(glm::mat4(1.0f), transform.Rotation.z + 0.0f, glm::vec3(0.0f, 0.0f, 1.0f)) *
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

                    auto projectileView = m_registry.view<ProjectileComponent, TransformComponent, IDComponent>();

                    for (auto projectileEntity : projectileView)
                    {

                        auto [projectileTransform, projectile, IDComp] = projectileView.get<TransformComponent, ProjectileComponent, IDComponent>(projectileEntity);
                        glm::vec2 projectilePos;
                        projectilePos.x = projectileTransform.Translation.x;
                        projectilePos.y = projectileTransform.Translation.y;
                        glm::vec2 randomOffset = glm::vec2(0.0f, 1.0f);
                        projectilePos = projectilePos - randomOffset;
                        const float backOffset = 0.5f;

                        // make sure direction is normalized (optional if you already guarantee it)
                        glm::vec2 dir = projectile.Direction;
                        if (glm::length2(dir) > 0.0f)
                            dir = glm::normalize(dir);

                        // move slightly *behind* the direction
                        glm::vec2 projectilePosTipOffset = projectilePos;
                        projectilePosTipOffset += dir * backOffset;

                        // make struct

                        Engine::VulkanRenderer2D::CalculateCircleCollision(projectilePosTipOffset, projectile.ProjectileRadius, IDComp.ID,
                            eCollisionType::PROJECTILE, projectile.Damage, projectile.DestructionRadius, projectile.Direction,
                            projectile.TargetPositionAtFireTime, projectile.DistanceToTargetatFireTime, projectile.TargetPositionHeightZ1);

                        float zKey = 0.0f;
                        float rotation = std::atan2(projectile.Direction.y, projectile.Direction.x);


                        VulkanRenderer2D::GetBindlessDescriptorSetRenderer()->AddInstance(projectilePos, zKey, projectile.renderSlot, rotation);

                    }

                }

                {
                    //EE_PROFILE_SCOPE("throwables");

                    auto throwableView = m_registry.view<ThrowableComponent, TransformComponent, IDComponent>();

                    for (auto throwableEntity : throwableView)
                    {

                        auto [throwableTransform, throwable, IDComp] = throwableView.get<TransformComponent, ThrowableComponent, IDComponent>(throwableEntity);
                        glm::vec2 throwablePos;
                        throwablePos.x = throwableTransform.Translation.x;
                        throwablePos.y = throwableTransform.Translation.y;
                        glm::vec2 randomOffset = glm::vec2(0.0f, 1.0f);
                        throwablePos = throwablePos - randomOffset;
                        const float backOffset = 0.5f;



                        float zKey = 0.0f;


                        //  VulkanRenderer2D::GetBindlessDescriptorSetRenderer()->AddInstance(throwablePos, zKey, throwable.renderSlot, throwable.RotationZ);

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



                        //Engine::VulkanRenderer2D::DrawTextureQuad(model, chunkComp.VisualEffectTexture);
                        //Engine::VulkanRenderer2D::DrawVisualEffectTexture(model, chunkComp.VisualEffectTexture);

                    }
                }

            }

            /*
            glm::mat4 quadTransform = glm::mat4(1);
            quadTransform
            Engine::VulkanRenderer2D::DrawTextureQuad(quadTransform, Engine::AssetManager::GetTexture("wall_0019"));

            */

            // VulkanUIRenderer::DrawUIText(AssetManager::GetFont(), "HELLO \n WORLD", {20, 20}, {0.5f, 0.76f, 0.43f, 1}, 1.0f);


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
}