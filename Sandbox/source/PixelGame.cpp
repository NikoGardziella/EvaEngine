#include "pch.h"
#include "PixelGame.h"
#include <Engine/Debug/Instrumentor.h>
#include <Engine/AssetManager/AssetManager.h>
#include <Engine/Scene/SceneSerializer.h>

#include "Systems/Player/CharacterControllerSystem.h"
#include <Engine/Scene/Components/Player/CharacterControllerComponent.h>

#include "Systems/Collision/PixelCollisionSystem.h"

#include <imgui/imgui.h>
#include <glm/ext/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include "Systems/Combat/ProjectileSystem.h"
#include "Systems/Collision/PlayerCollisionSystem.h"
#include "Systems/Combat/HealthSystem.h"
#include "Systems/NPC/NpcAIMovementSystem.h"
#include "Systems/NPC/NPCAIVisionSystem.h"
#include "Systems/Player/PlayerMovementSystem.h"
#include "Systems/Player/Camera/PlayerCameraSystem.h"
#include <Engine/Scene/Components/Combat/WeaponComponent.h>
#include "Systems/Combat/PlayerWeaponSystem.h"


PixelGame::PixelGame(const std::string scene)
	: Layer("PixelGame"),
	m_orthoCameraController(1280.0f / 720.0f, true),
	m_activeSceneName(scene)
{
	m_activeScene = std::make_shared<Engine::Scene>();


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

	
}

void PixelGame::OnAttach()
{
	EE_PROFILE_FUNCTION();
	LoadGameAssets();

	m_orthoCameraController.SetZoomLevel(10.0f);

	Engine::SceneSerializer serializer(m_activeScene);
	std::string scenePath = Engine::AssetManager::GetScenePath(m_activeSceneName).string();
	if (!serializer.Deserialize(scenePath))
	{
		EE_CORE_ERROR("Failed to load scene at: {}", scenePath);
	}
	
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

void PixelGame::OnEvent(Engine::Event& event)
{
	m_orthoCameraController.OnEvent(event);

}

void PixelGame::OnGameStart()
{

	CreateGameEntities();
	m_activeScene->OnRunTimeStart();
	
	m_isPlaying = true;
}

void PixelGame::LoadGameAssets()
{
	m_pixelTexture = Engine::AssetManager::GetPixelTexture("pixel");
	m_playerTexture = Engine::AssetManager::GetTexture("player");
}

void PixelGame::OnGameStop()
{

	if (m_activeSceneName.empty())
	{
		m_activeSceneName = "currentScene";
		
	}
	Engine::SceneSerializer serializer(m_activeScene);
	serializer.Deserialize(Engine::AssetManager::GetScenePath(m_activeSceneName).string());

	m_activeScene->ClearRegistry();
}

void PixelGame::CreateGameEntities()
{
	/*
	if (!m_playerEntity)
	{
		m_playerEntity = m_activeScene->CreateEntity("player");
		auto& transformComp = m_playerEntity.AddComponent<Engine::TransformComponent>();
		transformComp.Translation += glm::vec3(0.0f, 5.0f, 0.0f);
		m_playerEntity.AddComponent<CharacterControllerComponent>();
		m_playerEntity.AddComponent<WeaponComponent>();
		m_playerEntity.AddComponent<Engine::CircleCollider2DComponent>();
		glm::vec4 color = { 1.0, 1.0, 1.0, 1.0f };
		Engine::SpriteRendererComponent& spriteComp = m_playerEntity.AddComponent<Engine::SpriteRendererComponent>();
		spriteComp.Color = color;
		spriteComp.Texture = m_playerTexture;

	}
	*/
	
	m_cameraEntity = m_activeScene->CreateEntity("camera");
	auto& cameraComp = m_cameraEntity.AddComponent<Engine::CameraComponent>();
	cameraComp.FixedAspectRatio = true;
	cameraComp.Camera.SetProjectionType(Engine::SceneCamera::ProjectionType::Perspective);
	cameraComp.Camera.SetPerspectiveFOV(45.0f);
	cameraComp.Primary = true;
	cameraComp.Camera.SetViewportBounds(m_activeScene->GetViewportMinBounds());

	auto& cameraTransformComp = m_cameraEntity.AddComponent<Engine::TransformComponent>();
	cameraTransformComp.Translation += glm::vec3(0.0f, 0.0f, 30.0f);

	


}


