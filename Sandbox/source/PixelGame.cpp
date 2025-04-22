#include "pch.h"
#include "PixelGame.h"
#include <Engine/Debug/Instrumentor.h>
#include <Engine/AssetManager/AssetManager.h>
#include <Engine/Scene/SceneSerializer.h>

#include <imgui/imgui.h>
#include <glm/ext/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>


PixelGame::PixelGame(std::string scene)
	: Layer("PixelGame"),
	m_orthoCameraController(1280.0f / 720.0f, true),
	m_activeSceneName(scene)
{
	m_activeScene = std::make_shared<Engine::Scene>();

}

void PixelGame::OnAttach()
{
	EE_PROFILE_FUNCTION();

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


	Engine::VulkanRenderer2D::ResetStats();
	{
		EE_PROFILE_SCOPE("render pre");

	}

	{
		//m_framebuffer->ClearColorAttachment(1, -1)
		if (m_isPlaying)
		{
			m_activeScene->OnUpdateRuntime(timestep, m_isPlaying);
		}
	}
}

void PixelGame::OnEvent(Engine::Event& event)
{
	m_orthoCameraController.OnEvent(event);

}

void PixelGame::OnGameStart()
{
	
	m_squareEntity = m_activeScene->CreateEntity("square");
	m_squareEntity.AddComponent<Engine::TransformComponent>();
	m_squareEntity.AddComponent<Engine::SpriteRendererComponent>();


	m_cameraEntity = m_activeScene->CreateEntity("camera");
	auto& cameraComp = m_cameraEntity.AddComponent<Engine::CameraComponent>();
	cameraComp.FixedAspectRatio = true;
	cameraComp.Camera.SetProjectionType(Engine::SceneCamera::ProjectionType::Perspective);
	cameraComp.Camera.SetPerspectiveFOV(45.0f);

	auto& cameraTransformComp = m_cameraEntity.AddComponent<Engine::TransformComponent>();
	cameraTransformComp.Translation += glm::vec3(0.0f, 0.0f, 80.0f);

	m_activeScene->OnRunTimeStart();

}

void PixelGame::OnGameStop()
{
	m_activeScene->ClearRegistry();

	Engine::SceneSerializer serializer(m_activeScene);
	serializer.Deserialize(Engine::AssetManager::GetAssetPath(m_activeSceneName).string());
}


