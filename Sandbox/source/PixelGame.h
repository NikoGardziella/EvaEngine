#pragma once

#include "Engine.h"
#include <string>
#include <Engine/Platform/Vulkan/Pixel/VulkanPixelTexture.h>

class PixelGame : public Engine::Layer
{


public:

	PixelGame(const std::string scene = "");
	virtual ~PixelGame() = default;

	virtual void OnAttach() override;
	virtual void OnDetach() override;
	virtual void OnImGuiRender() override;

	void OnUpdate(Engine::Timestep timestep) override;
	void OnEvent(Engine::Event& event) override;
	void OnGameStart();
	void LoadGameAssets();
	void OnGameStop();


	Engine::Ref<Engine::Scene>& GetActiveGameScene() { return m_activeScene; }

	void SetIsPlaying(bool play) { m_isPlaying = play; }
	void SetActiveScene(Engine::Ref<Engine::Scene>& scene) { m_activeScene = Engine::Scene::Copy(scene); }

	std::string GetActiveSceneName() { return m_activeSceneName; }


public:


private:

	Engine::Ref<Engine::Scene> m_activeScene;
	Engine::Entity m_cameraEntity;
	Engine::Entity m_playerEntity;

	bool m_isPlaying = false;
	std::string m_activeSceneName;
	Engine::OrthographicCameraController m_orthoCameraController;
	Engine::Entity m_pixelEntity;

	Engine::Ref<Engine::VulkanPixelTexture> m_pixelTexture;
	Engine::Ref<Engine::VulkanTexture> m_logoTexture;

	float m_timer = 0.0f;

};


