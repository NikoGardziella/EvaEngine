#include "pch.h"
#include "PixelGame.h"
#include <Engine/Debug/Instrumentor.h>
#include <Engine/AssetManager/AssetManager.h>
#include <Engine/Scene/SceneSerializer.h>

#include "Systems/Player/CharacterControllerSystem.h"
#include "Systems/Collision/PixelCollisionSystem.h"

#include <imgui/imgui.h>
#include <glm/ext/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>


PixelGame::PixelGame(const std::string scene)
	: Layer("PixelGame"),
	m_orthoCameraController(1280.0f / 720.0f, true),
	m_activeSceneName(scene)
{
	m_activeScene = std::make_shared<Engine::Scene>();


	m_activeScene->RegisterSystem(CharacterControllerSystem::UpdateCharacterControllerSystem);
	m_activeScene->RegisterSystem(PixelCollisionSystem::UpdatePixelCollisionSystem);

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


	//m_logoTexture = Engine::AssetManager::GetTexture("logo");
	

	
	
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

	m_timer += timestep;
	if (m_timer < 0.0f)
	{
		return;
	}
	m_timer = 0.0f;

	EE_PROFILE_FUNCTION();
	{
		m_orthoCameraController.OnUpdate(timestep);
	}
	
	//Engine::VulkanRenderer2D::ResetStats();
	{
		EE_PROFILE_SCOPE("render pre");

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


			//Engine::VulkanRenderer2D::DrawTextureQuad(transform, m_pixelTexture, 1, color);
			
			//Engine::TransformComponent& cameraTransformComp = m_cameraEntity.GetComponent<Engine::TransformComponent>();

			Engine::VulkanRenderer2D::BeginScene();
			Engine::VulkanRenderer2D::DrawQuad(transform, color, -1);
			Engine::VulkanRenderer2D::EndScene();
			
			
			

			//m_pixelTexture->SetPixel(0, 1, 255, 255, 255, 0);
			//m_pixelTexture->ApplyChanges();

		}
	}
}

void PixelGame::OnEvent(Engine::Event& event)
{
	m_orthoCameraController.OnEvent(event);

}

void PixelGame::OnGameStart()
{

	//m_squareEntity = m_activeScene->CreateEntity("square");
	//m_squareEntity.AddComponent<Engine::TransformComponent>();
	//m_squareEntity.AddComponent<Engine::SpriteRendererComponent>();

	// move this to new function called LoadTextures. This is called on app creation

	m_pixelEntity = m_activeScene->CreateEntity("pixel entity");
	m_pixelEntity.AddComponent<Engine::TransformComponent>();

	//m_logoEntity.AddComponent<Engine::SpriteRendererComponent>();
	auto& renderComp = m_pixelEntity.AddComponent<Engine::PixelSpriteRendererComponent>();
	renderComp.Texture = m_pixelTexture;


	m_activeScene->OnRunTimeStart();
	m_cameraEntity = m_activeScene->CreateEntity("camera");
	auto& cameraComp = m_cameraEntity.AddComponent<Engine::CameraComponent>();
	cameraComp.FixedAspectRatio = true;
	cameraComp.Camera.SetProjectionType(Engine::SceneCamera::ProjectionType::Perspective);
	cameraComp.Camera.SetPerspectiveFOV(45.0f);
	cameraComp.Primary = true;

	auto& cameraTransformComp = m_cameraEntity.AddComponent<Engine::TransformComponent>();
	cameraTransformComp.Translation += glm::vec3(0.0f, 0.0f, 30.0f);

	m_playerEntity = m_activeScene->CreateEntity("player");

	auto& transformComp = m_playerEntity.AddComponent<Engine::TransformComponent>();
	transformComp.Translation += glm::vec3(0.0f, 5.0f, 0.0f);
	m_playerEntity.AddComponent<Engine::CharacterControllerComponent>();

	glm::vec4 color = { 0.2, 0.9, 0.8, 1.0f };
	Engine::SpriteRendererComponent& spriteComp = m_playerEntity.AddComponent<Engine::SpriteRendererComponent>();
	spriteComp.Color = color;
	spriteComp.Texture = m_playerTexture;

	Engine::SceneCamera Camera = m_cameraEntity.GetComponent<Engine::CameraComponent>().Camera;

	m_isPlaying = true;
}

void PixelGame::LoadGameAssets()
{
	m_pixelTexture = Engine::AssetManager::GetPixelTexture("pixel");
	m_playerTexture = Engine::AssetManager::GetTexture("chess");
}

void PixelGame::OnGameStop()
{
	m_activeScene->ClearRegistry();

	Engine::SceneSerializer serializer(m_activeScene);
	serializer.Deserialize(Engine::AssetManager::GetAssetPath(m_activeSceneName).string());
}


