#include "pch.h"


#include "Engine/Map/Grid/GridMap.h"
#include "Engine/Debug/DebugInterface.h"
#include "Engine/Map/Tile/TileManager.h"
#include "Component.h"
#include "Components/NPC/NpcAIComponent.h"


#include "Components/Spawning/NpcSpawnControllerComponent.h"

#include <Engine/Map/FogOfWar/FogOfWar.h>

#include <Engine/Animation/3D/System/CullingSystem3D.h>
#include <Engine/Animation/2D/AnimationSystem2D.h>
#include <Engine/Animation/3D/System/TransformSystem3D.h>
#include <Engine/Animation/2D/AnimationBank2D.h>
#include "Components/Light/DirectionalLightComponent.h"
#include "Components/Light/PointLightComponent.h"
#include "Components/Light/SpotLightComponent.h"
#include "Engine/AssetManager/AssetManager.h"
#include "Components/Render/3D/MeshRefComponent.h"
#include "Components/Render/3D/RenderBoundsComponent.h"
#include "Components/Render/3D/SkeletonComponent.h"
#include "Components/Render/3D/AnimatorComponent.h"
#include "Components/Combat/EquippedWeaponComponent.h"
#include "Components/Animation/PlayerAnimation/CharacterAnimSetComponent.h"
#include "Components/Animation/PlayerAnimation/CharacterAnimStateComponent.h"
#include "Components/UI/HUDStateComponent.h"
#include "Components/Player/PlayerVisionComponent.h"
#include "Components/Player/CharacterControllerComponent.h"
#include "Components/Environment/DayNightComponent.h"

namespace Engine {

	uint32_t PROFILING = 0;


    Scene::Scene()
    {
		EE_CORE_INFO("Creating new Scene");
        m_registry = entt::registry();
        m_gridMap = std::make_shared<GridMap>();

        m_fogOfWar = std::make_shared<FogOfWar>(m_gridMap); // after gridmap

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




    void Scene::SpawnPlayer()
    {

        EE_CORE_WARN("move this stuff somewher");
        MeshRegistry& meshReg = AssetManager::GetMeshRegistry();

        const MeshAsset* meshAsset = meshReg.GetMeshByKey("playerMeshes");

        const uint32_t submeshCount = (uint32_t)meshAsset->submeshes.size();

        Entity playerEntity = CreateEntity("player");

        Engine::CircleCollider2DComponent& circleCollider2DComponent = playerEntity.AddComponent<Engine::CircleCollider2DComponent>();
        TransformComponent& transformComp = playerEntity.AddComponent<TransformComponent>();
        WeaponComponent& weaponComp = playerEntity.AddComponent<WeaponComponent>();
        CharacterControllerComponent& characterControllerCompo = playerEntity.AddComponent<CharacterControllerComponent>();


        MeshRefComponent& meshComp = playerEntity.AddComponent<MeshRefComponent>();
        meshComp.meshId = meshAsset->id;
        meshComp.submeshFirst = 0;
        meshComp.submeshCount = submeshCount;



        RenderBoundsComponent& renderBoundsComp = playerEntity.AddComponent<RenderBoundsComponent>();
        renderBoundsComp.maxL = meshAsset->maxL;
        renderBoundsComp.minL = meshAsset->minL;

        SkeletonComponent& skel = playerEntity.AddComponent<SkeletonComponent>();
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
        playerTransformComp.Translation.z = 0.01f;
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


    void Scene::OnRunTimeStart()
    {

        EE_CORE_INFO("Starting runtime!");



        m_lightGatherSystem.Update(this);






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



        m_cullingSystem3D = std::make_shared<CullingSystem3D>();
        m_transformSystem3D = std::make_shared<TransformSystem3D>();

        Entity spwanController = CreateEntity("spawn controller");
        NpcSpawnControllerComponent& npcSpawnControllerComponent  = spwanController.AddComponent<NpcSpawnControllerComponent>();
        npcSpawnControllerComponent.maxAlive = 0;
        //npcSpawnControllerComponent.spawnInterval = 0.001f;

        Entity Light = CreateEntity("Light");
        DirectionalLightComponent& lightComp = Light.AddComponent<DirectionalLightComponent>();
        TransformComponent& lightTransformComp = Light.AddComponent<TransformComponent>();
        //lightComp.directionWS = glm::vec3(0.5f, -1.0f, 0.5f);  // Down and diagonal
        lightComp.color = glm::vec3(1.0f, 1.0f, 1.0f);
        lightComp.intensity = 1.0f;

        lightTransformComp.Translation.z = 20.0f;
        //lightTransformComp.Scale = glm::vec3(3);

        //lightComp.directionWS = glm::normalize(glm::vec3(0.0f, 0.0f, -1.0f));
        lightComp.directionWS = glm::normalize(glm::vec3(0.0f, -0.7f, -0.7f));
       
        SpawnPlayer();

        Entity sun = CreateEntity("DayNight");
        auto& dn = sun.AddComponent<DayNightComponent>();
        dn.dayLengthSeconds = 600.0f;
        dn.timeNormalized = 0.35f; // ~8:24 AM

    }






    void Scene::OnRunTimeStop()
    {
        ForEach<DirectionalLightComponent>([&](Entity e, DirectionalLightComponent& dl)
            {
                // Destroy editor camera.
                if (e.HasComponent<TransformComponent>())
                {
                    e.RemoveComponent<TransformComponent>();
                }
                DestroyEntity(e);
            });
        ForEach<CharacterControllerComponent>([&](Entity e, CharacterControllerComponent& caracterControllerComp)
            {
                // Destroy player
                if (e.HasComponent<TransformComponent>())
                {
                    
                    e.RemoveComponent<TransformComponent>();
                }
                DestroyEntity(e);
            });


        m_textureStreamingSystem->UnloadAllChunks(this);
        m_tileMananger->Shutdown();
    }

    void Scene::PauseRuntime()
    {

        // TODO save all velocities and add them on resume
    }


    void Scene::ResumeRuntime()
    {

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
            CameraComponent& cameraComp = view.get<CameraComponent>(entity);
            if (!cameraComp.FixedAspectRatio)
            {
                cameraComp.Camera.SetViewportSize(width, height);
                cameraComp.Camera.SetViewportBounds(viewportBounds);
                
            }

        }


    }


  

    void Scene::RegisterSystem(const std::function<void(float, Scene* scene)>& system)
    {
        m_gameplaySystems.emplace_back(system);
        EE_CORE_INFO("System registered");
    }

    
}
