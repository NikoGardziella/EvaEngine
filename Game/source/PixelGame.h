#pragma once

#include "Engine.h"
#include <string>
#include <Engine/UI/UIContext.h>

class PixelGame : public Engine::Layer
{


public:

	PixelGame(const std::string scene = "");
	virtual ~PixelGame() = default;

	virtual void OnAttach() override;
	virtual void OnDetach() override;
	virtual void OnImGuiRender() override;

	void OnUpdate(Engine::Timestep timestep) override;
	void OnUpdateECS(Engine::Timestep timestep) override;
	void OnEvent(Engine::Event& event) override;
	void OnGameStart();
	void LoadGameAssets();
	void OnGameStop();
	void CreateGameEntities();

	void SpawnChunkGridSprites();
	void RegisterSystems();

	Engine::Ref<Engine::Scene>& GetActiveGameScene() { return m_activeScene; }

	void SetIsPlaying(bool play) { m_isPlaying = play; }
	void CopyToActiveScene(Engine::Ref<Engine::Scene>& scene) { m_activeScene = Engine::Scene::Copy(scene); }
	void SetActiveScene(Engine::Ref<Engine::Scene>& scene) { m_activeScene = scene; };

	std::string GetActiveSceneName() { return m_activeSceneName; }

	void SetViewportSize(uint32_t width, uint32_t height) { m_viewportHeight = height, m_viewportWidth = width;  }

public:


private:

	Engine::Ref<Engine::Scene> m_activeScene;
	Engine::Entity m_cameraEntity;
	Engine::Entity m_uiCameraEntity;
	//Engine::Entity m_playerEntity;

	bool m_isPlaying = false;
	std::string m_activeSceneName;
	Engine::OrthographicCameraController m_orthoCameraController;
	Engine::Entity m_pixelEntity;

	Engine::Ref<Engine::VulkanTexture> m_logoTexture;
	//Engine::Ref<Engine::VulkanTexture> m_playerTexture;

	uint32_t m_viewportWidth;
	uint32_t m_viewportHeight;

	// Map



	//UI
	

};


