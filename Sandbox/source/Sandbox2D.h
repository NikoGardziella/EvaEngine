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

private:

	Engine::OrthographicCameraController m_orthoCameraController;
	Engine::Ref<Engine::Shader> m_flatColorShader;
	glm::vec4 m_squareColor = { 0.2f, 0.3f, 0.8f, 1.0f };
	Engine::Ref<Engine::VertexArray> m_squareVA;

	Engine::Ref<Engine::Texture2D> m_checkerBoardTexture;
	Engine::Ref<Engine::Texture2D> m_spriteSheet;


};

