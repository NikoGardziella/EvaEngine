#pragma once


#include "Engine.h"

class Sandbox2D : public Engine::Layer
{


public:

	Sandbox2D(std::string scene = "");
	virtual ~Sandbox2D() = default;

	virtual void OnAttach() override;
	virtual void OnDetach() override;
	virtual void OnImGuiRender() override;

	void OnUpdate(Engine::Timestep timestep) override;
	void OnEvent(Engine::Event& event) override;
	void OnGameStart();
	void OnGameStop();

	void CreateTestScene();

	Engine::Ref<Engine::Scene>& GetActiveGameScene() { return m_activeScene; }
	Engine::Ref<Engine::Framebuffer>& GetGameFramebuffer() { return m_framebuffer; }

	void SetIsPlaying(bool play) { m_isPlaying = play; }
	void SetActiveScene(Engine::Ref<Engine::Scene>& scene) { m_activeScene = Engine::Scene::Copy(scene); }
	
	std::string GetActiveSceneName() { return m_activeSceneName; }


public:


private:

	Engine::OrthographicCameraController m_orthoCameraController;
	Engine::Ref<Engine::Shader> m_flatColorShader;
	glm::vec4 m_squareColor = { 0.2f, 0.3f, 0.8f, 1.0f };


	Engine::Ref<Engine::Framebuffer> m_framebuffer;

	Engine::Ref<Engine::Texture2D> m_checkerBoardTexture;
	Engine::Ref<Engine::Texture2D> m_spriteSheet;
	Engine::Ref<Engine::Texture2D> m_textureSpriteSheetPacked;

	Engine::Ref<Engine::VulkanTexture> m_texture;


	Engine::Ref<Engine::SubTexture2D> m_textureSprite;
	Engine::Ref<Engine::SubTexture2D> m_textureBarrel;

	//Engine::Ref<Engine::Scene> m_gameScene;

	//uint32_t m_mapWidth;
	//uint32_t m_mapHeight;


	std::unordered_map<char, Engine::Ref<Engine::SubTexture2D>> m_textureMap;


	Engine::Ref<Engine::Scene> m_activeScene;
	Engine::Entity m_cameraEntity;
	Engine::Entity m_squareEntity;
	
	bool m_isPlaying = false;
	std::string m_activeSceneName;
};


