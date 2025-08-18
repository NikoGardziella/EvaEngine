#include "pch.h"
#include "PixelGame.h"
#include <Engine/Debug/Instrumentor.h>
#include <Engine/AssetManager/AssetManager.h>
#include <Engine/Scene/SceneSerializer.h>

#include "Systems/Player/CharacterControllerSystem.h"

#include "Systems/Collision/PixelCollisionSystem.h"

#include <glm/ext/matrix_transform.hpp>
#include "Systems/Combat/ProjectileSystem.h"
#include "Systems/Collision/PlayerCollisionSystem.h"
#include "Systems/Collision/VehicleCollisionSystem.h"
#include "Systems/Combat/HealthSystem.h"
#include "Systems/NPC/NpcAIMovementSystem.h"
#include "Systems/NPC/NPCAIVisionSystem.h"
#include "Systems/Player/PlayerMovementSystem.h"
#include "Systems/Player/Camera/PlayerCameraSystem.h"
#include "Systems/Combat/PlayerWeaponSystem.h"
#include "Systems/Vehicle/VehicleSystem.h"


PixelGame::PixelGame(const std::string scene)
	: Layer("PixelGame"),
	m_orthoCameraController(1280.0f / 720.0f, true),
	m_activeSceneName(scene)
{
	m_activeScene = std::make_shared<Engine::Scene>();

	
	
	

}

void PixelGame::OnAttach()
{
	EE_PROFILE_FUNCTION();


	LoadGameAssets();

	m_orthoCameraController.SetZoomLevel(30.0f);

	
	
}

void PixelGame::RegisterSystems()
{
	// -----------------------------------------------------------------------------
	// Order of the systems matter! for example:
	// 1. Check input from player
	// 2. Check collisions
	// 3. apply translation accordingly
	// 
	// -----------------------------------------------------------------------------
	m_activeScene->RegisterSystem(CharacterControllerSystem::UpdateCharacterControllerSystem);

	m_activeScene->RegisterSystem(PlayerCollisionSystem::UpdatePlayerCollision);
	m_activeScene->RegisterSystem(PlayerMovementSystem::MovementSystem);
	m_activeScene->RegisterSystem(PlayerCameraSystem::UpdatePlayerCameraSystem);
	m_activeScene->RegisterSystem(PlayerWeaponSystem::UpdatePlayerWeaponSystem);

	m_activeScene->RegisterSystem(PixelCollisionSystem::UpdatePixelCollisionSystem);
	m_activeScene->RegisterSystem(ProjectileSystem::UpdateProjectileSystem);
	m_activeScene->RegisterSystem(HealthSystem::UpdateHealthSystem);
	m_activeScene->RegisterSystem(NpcAIMovementSystem::UpdateNPCAIMovementSystem);
	m_activeScene->RegisterSystem(NPCAIVisionSystem::UpdateNPCAIVisionSystem);
	m_activeScene->RegisterSystem(VehicleCollisionSystem::UpdateVehicleCollision);
	m_activeScene->RegisterSystem(VehicleSystem::UpdateVehicleSystem);
}

void PixelGame::OnDetach()
{
	EE_PROFILE_FUNCTION();

}

void PixelGame::OnImGuiRender()
{
	EE_PROFILE_FUNCTION();



}

void PixelGame::OnUpdate(Engine::Timestep timestep)
{
	EE_PROFILE_FUNCTION();

	{
		m_orthoCameraController.OnUpdate(timestep);
	}
	

	{
		//m_framebuffer->ClearColorAttachment(1, -1)
		if (m_isPlaying)
		{
			

			m_activeScene->OnUpdateECSRuntime(timestep);
			m_activeScene->OnUpdateRuntime(timestep, m_isPlaying);

			const glm::mat4 viewProjection = m_orthoCameraController.GetCamera().GetViewProjectionMatrix();
					
			glm::vec2 position = { 5.9f, 0.7f };
			glm::vec2 size = { 10.0f, 4.0f }; // Width = 2, Height = 3
			glm::vec4 color = { 0.1f, 0.9f, 0.1f, 1.0f };
			glm::mat4 transform = glm::translate(glm::mat4(1.0f), { position.x, position.y, 0.0f }) *
				glm::scale(glm::mat4(1.0f), { size.x, size.y, 1.0f });



		}
	}
}

void PixelGame::OnUpdateECS(Engine::Timestep timestep)
{
	m_activeScene->OnUpdateECSRuntime(timestep);
}

void PixelGame::OnEvent(Engine::Event& event)
{
	m_orthoCameraController.OnEvent(event);

}

void PixelGame::OnGameStart()
{
	
	RegisterSystems();
	CreateGameEntities();
	m_activeScene->OnRunTimeStart();
	
	m_isPlaying = true;
	auto& cameraComp = m_cameraEntity.GetComponent<Engine::CameraComponent>();
	cameraComp.Camera.SetPerspectiveFOV(45.0f);

}

void PixelGame::LoadGameAssets()
{
	m_pixelTexture = Engine::AssetManager::GetPixelTexture("pixel");
	//m_playerTexture = Engine::AssetManager::GetTexture("player");

	Engine::SceneSerializer serializer(m_activeScene);
	std::string scenePath = Engine::AssetManager::GetScenePath(m_activeSceneName).string();
	if (!serializer.Deserialize(scenePath))
	{
		EE_CORE_ERROR("Failed to load scene at: {}", scenePath);
	}

}

void PixelGame::OnGameStop()
{

	if (m_activeSceneName.empty())
	{
		m_activeSceneName = "currentScene";
		
	}
	//m_activeScene->ClearRegistry();
	
	//Engine::SceneSerializer serializer(m_activeScene);
	//serializer.Deserialize(Engine::AssetManager::GetScenePath(m_activeSceneName).string());

}

void PixelGame::CreateGameEntities()
{
	
	/*
		m_playerEntity = m_activeScene->CreateEntity("player");
		auto& playerTransformComp = m_playerEntity.AddComponent<Engine::TransformComponent>();
		playerTransformComp.Translation += glm::vec3(0.0f, 0.0f, 0.0f);
		m_playerEntity.AddComponent<CharacterControllerComponent>();
		m_playerEntity.AddComponent<WeaponComponent>();
		m_playerEntity.AddComponent<Engine::CircleCollider2DComponent>();
		//glm::vec4 color = { 1.0, 1.0, 1.0, 1.0f };
		Engine::SpriteRendererComponent& playerSpriteComp = m_playerEntity.AddComponent<Engine::SpriteRendererComponent>();
		//playerSpriteComp.Color = color;
		playerSpriteComp.Texture = m_playerTexture;

		m_pixelEntity = m_activeScene->CreateEntity("pixel entity");
		auto& transformComp = m_pixelEntity.AddComponent<Engine::TransformComponent>();
		transformComp.Translation += glm::vec3(0.0f, 5.0f, 0.0f);
		Engine::SpriteRendererComponent& spriteComp = m_pixelEntity.AddComponent<Engine::SpriteRendererComponent>();
		spriteComp.Texture = m_pixelTexture;

	

	*/
	
	m_cameraEntity = m_activeScene->CreateEntity("camera");
	auto& cameraComp = m_cameraEntity.AddComponent<Engine::CameraComponent>();
	cameraComp.FixedAspectRatio = true;
	cameraComp.Camera.SetProjectionType(Engine::SceneCamera::ProjectionType::Perspective);
	cameraComp.Camera.SetPerspectiveFOV(45.0f);
	cameraComp.Primary = true;
	cameraComp.FreeCamera = false;
	cameraComp.Camera.SetViewportBounds(m_activeScene->GetViewportBounds());


	cameraComp.Camera.SetViewportSize(m_activeScene->GetViewportWidth(), m_activeScene->GetViewortHeight());
	
	auto& cameraTransformComp = m_cameraEntity.AddComponent<Engine::TransformComponent>();
	cameraTransformComp.Translation += glm::vec3(0.0f, 0.0f, 15.0f);

	//SpawnChunkGridSprites();

}


// In your DebugPanel or wherever you want to trigger it:

void PixelGame::SpawnChunkGridSprites()
{
	// Optional: clean up previous debug sprites by tag

	constexpr int mapWidth = 2048;   // total world size in pixels
	constexpr int mapHeight = 2048;
	constexpr int chunkSize = 64;    // size of each cell

	const int cols = mapWidth / chunkSize;  // 16
	const int rows = mapHeight / chunkSize;  // 16

	for (int cy = 0; cy < rows; cy++)
	{
		for (int cx = 0; cx < cols; cx++)
		{
			// world-space origin of this chunk cell
			glm::vec2 origin = {
				float(cx * chunkSize),
				float(cy * chunkSize)
			};

			// Create and tag the debug entity
			auto e = m_activeScene->CreateEntity("ChunkDebug");

			// Place the sprite at the cell origin
			auto& tf = e.AddComponent<Engine::TransformComponent>();
			tf.Translation = glm::vec3(origin, 0.0f);

			// Give it a tiny 16×16 sprite so you can spot it
			auto& spr = e.AddComponent<Engine::SpriteRendererComponent>();
			spr.Texture = Engine::AssetManager::GetTexture("wall_0019");

		}
	}
}


