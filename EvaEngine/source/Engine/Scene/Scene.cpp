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





    void Scene::OnRunTimeStart()
    {

        EE_CORE_INFO("Starting runtime!");

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

       // Entity spwanController = CreateEntity("spawn controller");
       // spwanController.AddComponent<NpcSpawnControllerComponent>();


        Entity Light = CreateEntity("Light");
        PointLightComponent& lightComp = Light.AddComponent<PointLightComponent>();
        TransformComponent& lightTransformComp = Light.AddComponent<TransformComponent>();
        //lightComp.radius = 50.0f;
        //lightTransformComp.Translation.x = 10.0f;
        // UI


       
        
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
