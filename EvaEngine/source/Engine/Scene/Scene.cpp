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
#include "Components/Vehicles/VehicleComponent.h"
#include "Engine/Map/Utils/IsoTileUtils.h"

#include "../../../../Editor/source/EditorLayer.h"
#include "../../../../Editor/source/Panels/Utils/EditorUtils.h"
#include <Engine/Math/HashUtils.h>
#include <Engine/Map/Tile/CompactTileMap.h>
#include <Engine/Map/Tile/TileDefinitionRegistry.h>


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
        m_box2DPhysicsSystem.Init();

    }


    Scene::~Scene()
    {
         
    }




    void Scene::SpawnPlayer()
    {

        EE_CORE_WARN("move this stuff somewher");
        MeshRegistry& meshReg = AssetManager::GetMeshRegistry();

        //const MeshAsset* meshAsset = meshReg.GetMeshByKey("playerMeshes");
        const MeshAsset* meshAsset = meshReg.GetMeshByKey("playerMeshes");

        const uint32_t submeshCount = (uint32_t)meshAsset->submeshes.size();

        Entity playerEntity = CreateEntity("player");
        EE_CORE_INFO("player ID {}", (uint32_t)playerEntity.Handle());

        Engine::CircleCollider2DComponent& circleCollider2DComponent = playerEntity.AddComponent<Engine::CircleCollider2DComponent>();
        TransformComponent& transformComp = playerEntity.AddComponent<TransformComponent>();
        WeaponComponent& weaponComp = playerEntity.AddComponent<WeaponComponent>();
        CharacterControllerComponent& characterControllerCompo = playerEntity.AddComponent<CharacterControllerComponent>();


        MeshRefComponent& meshComp = playerEntity.AddComponent<MeshRefComponent>();
        meshComp.meshId = meshAsset->id;
        meshComp.submeshFirst = 0;
        meshComp.submeshCount = submeshCount;
        EE_CORE_INFO("car mesh id{}", meshComp.meshId);


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


        //****************** spawn car ********************

        Entity carEntity = CreateEntity("car");
        const MeshAsset* meshAssetCar = meshReg.GetMeshByKey("PickUp");
        TransformComponent& carTransformComp = carEntity.AddComponent<TransformComponent>();
        carTransformComp.Rotation.x += glm::radians(90.0f);
        carTransformComp.Translation.x = 10.0f;
        carTransformComp.Scale = glm::vec3(0.3f, 0.3f, 0.3f);



        const uint32_t carsubmeshCount = (uint32_t)meshAssetCar->submeshes.size();


        MeshRefComponent& carMeshComp = carEntity.AddComponent<MeshRefComponent>();
        carMeshComp.meshId = meshAssetCar->id;
        carMeshComp.submeshFirst = 0;
        carMeshComp.submeshCount = carsubmeshCount;


        RenderBoundsComponent& carRenderBoundsComp = carEntity.AddComponent<RenderBoundsComponent>();
        carRenderBoundsComp.maxL = meshAssetCar->maxL;
        carRenderBoundsComp.minL = meshAssetCar->minL;

        SkeletonComponent& carSkel = carEntity.AddComponent<SkeletonComponent>();
        carSkel.skeletonId = meshAssetCar->skeletonId;      // returned by importer
        carSkel.boneCount = AssetManager::GetSkeletonRegistry().Get(meshAssetCar->skeletonId).parent.size();
        carSkel.boneBase = 0xFFFFFFFFu;     // let BonePalette allocate

        VehicleComponent& carVehicleComp = carEntity.AddComponent<VehicleComponent>();

        
    }

    void Scene::CreateLotsOfCompactTilesOnStartup(
        uint16_t typeId,
        int width,
        int height,
        const glm::ivec2& startIsoCell,
        uint64_t baseGroupId)
    {
        EE_PROFILE_FUNCTION();

        if (typeId == 0)
        {
            EE_CORE_WARN("CreateLotsOfCompactTilesOnStartup: invalid typeId 0");
            return;
        }

        if (width <= 0 || height <= 0)
        {
            EE_CORE_WARN("CreateLotsOfCompactTilesOnStartup: invalid size {} x {}", width, height);
            return;
        }

        CompactTileMap& compactMap = GetCompactTileMap();

        // One compact group per block
        constexpr int GROUP_BLOCK_W = 4;
        constexpr int GROUP_BLOCK_H = 4;

        int placedCount = 0;

        for (int y = 0; y < height; ++y)
        {
            for (int x = 0; x < width; ++x)
            {
                int stepX = 2; // 1 = no gap, 2 = 1 tile gap, 3 = 2 tile gap
                int stepY = 2;

                const glm::ivec2 isoCell = startIsoCell + glm::ivec2((x + 1) * stepX, (y + 1) * stepY);

                const int blockX = x / GROUP_BLOCK_W;
                const int blockY = y / GROUP_BLOCK_H;

                // Derive a unique group id per block
                const uint64_t groupId =
                    baseGroupId +
                    uint64_t(blockY) * 100000ull +
                    uint64_t(blockX);

                
            }
        }

        EE_CORE_INFO(
            "CreateLotsOfCompactTilesOnStartup: placed {} compact tiles, typeId={}, start=({}, {}), size={}x{}, baseGroupId={}, block={}x{}",
            placedCount, typeId, startIsoCell.x, startIsoCell.y, width, height,
            (uint64_t)baseGroupId, GROUP_BLOCK_W, GROUP_BLOCK_H);
    }

    void Scene::OnRunTimeStart()
    {
        EE_PROFILE_FUNCTION();
        EE_CORE_INFO("Starting runtime!");


       
        m_lightGatherSystem.Update(this);

        glm::vec4 uv = glm::vec4(0.1352539f, 0.619486f, 0.25927734f, 0.66596794f);

        //m_compactTilePromotion.PromoteAllTiles(this);
        {
            TileDefinitionRegistry& defs = GetTileDefinitions();

            TileTypeKey key{};
            key.name = "Wall B1_S";
            key.uv = glm::vec4(0.1352539f, 0.619486f, 0.25927734f, 0.66596794f);
            key.category = eTileCategory::Buildings;
            key.direction = eTileDirection::South;

            uint16_t typeId = 0;

            if (!defs.FindTypeId(key, typeId))
            {
                TileDefinition def{};
                def.TypeId = defs.GetNextTypeId();
                def.Name = "Wall B1_S";
                def.UV = glm::vec4(0.1352539f, 0.619486f, 0.25927734f, 0.66596794f);
                def.Category = eTileCategory::Buildings;
                def.Direction = eTileDirection::South;
                def.Material = eTileMaterial::None;
                def.BaseHealth = 10;
                def.IsDestructible = true;
                def.IsSupportingRoof = true;
                def.IsRoof = false;

                if (!defs.Register(def, key))
                {
                    EE_CORE_WARN("Failed to register debug compact tile type");
                    
                }

                typeId = def.TypeId;
            }

            //CreateLotsOfCompactTilesOnStartup(typeId, 1000, 1000, glm::ivec2(10, 10), 1);

            
        }
       

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
        dn.paused = true;


        m_destructibleTileSystem.InitDestructableSystem(this);



        m_box2DPhysicsSystem.OnRuntimeStart(this);
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

        m_box2DPhysicsSystem.OnRuntimeStop(this);

        PlayerData playerStateData;

        playerStateData.InGame = false;
        VulkanRenderer2D::SubmitPlayerData(playerStateData);
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
