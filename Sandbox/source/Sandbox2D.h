#pragma once


#include "Engine.h"

class Sandbox2D : public Engine::Layer
{


public:

	Sandbox2D();
	virtual ~Sandbox2D() = default;

	virtual void OnAttach() override;
	virtual void OnDetach() override;
	virtual void OnImGuiRender() override;

	void OnUpdate(Engine::Timestep timestep) override;
	void OnEvent(Engine::Event& event) override;

	Engine::Ref<Engine::Scene> GetActiveGameScene() { return m_activeScene; }
<<<<<<< HEAD
	Engine::Ref<Engine::Framebuffer> GetGameFramebuffer() { return m_framebuffer; }
=======
>>>>>>> ff0c3b600b617aa742d76fd6bff3b49a5a8e1cde

private:

	Engine::OrthographicCameraController m_orthoCameraController;
	Engine::Ref<Engine::Shader> m_flatColorShader;
	glm::vec4 m_squareColor = { 0.2f, 0.3f, 0.8f, 1.0f };


	Engine::Ref<Engine::Framebuffer> m_framebuffer;

	Engine::Ref<Engine::Texture2D> m_checkerBoardTexture;
	Engine::Ref<Engine::Texture2D> m_spriteSheet;
	Engine::Ref<Engine::Texture2D> m_textureSpriteSheetPacked;

	Engine::Ref<Engine::SubTexture2D> m_textureSprite;
	Engine::Ref<Engine::SubTexture2D> m_textureBarrel;

	//Engine::Ref<Engine::Scene> m_gameScene;

	uint32_t m_mapWidth;
	uint32_t m_mapHeight;


	std::unordered_map<char, Engine::Ref<Engine::SubTexture2D>> m_textureMap;


	Engine::Ref<Engine::Scene> m_activeScene;
	Engine::Entity m_cameraEntity;
	Engine::Entity m_squareEntity;
	


};


