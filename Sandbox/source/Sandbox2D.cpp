#include "Sandbox2D.h"
#include <imgui/imgui.h>

#include <glm/ext/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <Engine/Debug/Instrumentor.h>



Sandbox2D::Sandbox2D()
	: Layer ("Sandbox2D"),
	m_orthoCameraController(1280.0f / 720.0f, true)
{
}

void Sandbox2D::OnAttach()
{
	EE_PROFILE_FUNCTION();

	m_checkerBoardTexture = Engine::Texture2D::Create("assets/textures/chess_board.png");
	m_spriteSheet = Engine::Texture2D::Create("assets/textures/game/tilemap.png");
	
}

void Sandbox2D::OnDetach()
{
	EE_PROFILE_FUNCTION();

}

void Sandbox2D::OnImGuiRender()
{
	EE_PROFILE_FUNCTION();
	ImGui::Begin("Settings");

	auto stats = Engine::Renderer2D::GetStats();
	ImGui::Text("Renderer2D Stats:");
	ImGui::Text("Draw Calls: %d", stats.DrawCalls);
	ImGui::Text("Quads: %d", stats.QuadCount);
	ImGui::Text("Vertices: %d", stats.GetTotalVertexCount());
	ImGui::Text("Indicies: %d", stats.GetTotalIndexCount());

	ImGui::ColorEdit3("Square color", glm::value_ptr(m_squareColor));

  
	ImGui::End();
}

void Sandbox2D::OnUpdate(Engine::Timestep timestep)
{
	EE_PROFILE_FUNCTION();
    {
	    m_orthoCameraController.OnUpdate(timestep);
    }


	// ******** Render ***********

	// statistics
	Engine::Renderer2D::ResetStats();
    {
		EE_PROFILE_SCOPE("render pre");
	    Engine::RenderCommand::SetClearColor({ 0.2f, 0, 0.2f, 1 });
	    Engine::RenderCommand::Clear();
    }


    {

		static float rotation = 0.0f;
		rotation += timestep * 20.0f;

		EE_PROFILE_SCOPE("render draw");
	    Engine::Renderer2D::BeginScene(m_orthoCameraController.GetCamera());
	    Engine::Renderer2D::DrawQuad({ 0.5f, 0.5f }, {1.0f, 1.0f,}, { 0.8f, 0.3f, 0.3f, 1.0f});
	    Engine::Renderer2D::DrawQuad({ 1.5f, 1.5f }, {1.0f, 1.0f,}, { 0.2f, 0.3f, 0.9f, 1.0f});
	    Engine::Renderer2D::DrawRotatedQuad({ 1.0f,1.0f }, {1.0f, 1.0f,}, rotation, { 0.1f, 0.8f, 0.4f, 1.0f});
	    Engine::Renderer2D::DrawRotatedQuad({ 0.0f,0.0f, -0.1f }, {10.f, 10.f,}, 45.0f, m_checkerBoardTexture, 10.0f, glm::vec4(1.0f, 0.9f, 0.9f, 1.0f));
    }

	Engine::Renderer2D::EndScene();

	Engine::Renderer2D::BeginScene(m_orthoCameraController.GetCamera());
	Engine::Renderer2D::DrawRotatedQuad({ 0.0f,0.0f, 0.1f }, { 10.f, 10.f, }, 0.0f, m_spriteSheet, 1.0f, glm::vec4(1.0f, 0.9f, 0.9f, 1.0f));
	Engine::Renderer2D::EndScene();

}

void Sandbox2D::OnEvent(Engine::Event& event)
{
	m_orthoCameraController.OnEvent(event);

}


