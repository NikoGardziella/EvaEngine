#include "Sandbox2D.h"
#include <imgui/imgui.h>

#include <glm/ext/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <Engine/Debug/Instrumentor.h>

static const uint32_t s_mapWidth = 26;
static const char* s_mapTiles =
"WWWWWWWWWWWWWWWWWWWWWWWWWWW"
"WWWWWWDDDDWWWWWWWWWWWWWWWWW"
"WWWWDDDDDDDDDWWWWWWWWWWWWWW"
"WWWDDDDDDDDDDDDWWWWWWWWWWWW"
"WWDDDDDDDDDDDDDDWWWWWWWWWWW"
"WWDDDDDDDDDDDDDDDDWWWWWWWWW"
"WWWDDDDDDDDDDDDDWWWWWWWWWWW"
"WWWWWWWDDWWWWWWWWWWWWWWWWWW"
"WWWWDDDDDDDDDDDWWWWWWWWWWWW"
"WWWWWDDDDDDDDWWWWWWWWWWWWWW"
"WWWWWWWWWWWWWWWWWWWWWWWWWWW"
"WWWWWCWWWWWWWWWWWWWWWWWWWWW"
;

Sandbox2D::Sandbox2D()
	: Layer ("Sandbox2D"),
	m_orthoCameraController(1280.0f / 720.0f, true)
{
}

void Sandbox2D::OnAttach()
{
	EE_PROFILE_FUNCTION();

	m_checkerBoardTexture = Engine::Texture2D::Create("assets/textures/chess_board.png");
	m_textureSpriteSheetPacked = Engine::Texture2D::Create("assets/textures/game/RPGpack_sheet_2X.png");

	m_mapWidth = s_mapWidth;
	m_mapHeight = strlen(s_mapTiles) / s_mapWidth;
	m_textureMap['D'] = Engine::SubTexture2D::CreateFromCoordinates(m_textureSpriteSheetPacked, {6, 11}, {128,128});
	m_textureMap['W'] = Engine::SubTexture2D::CreateFromCoordinates(m_textureSpriteSheetPacked, {11, 11}, {128,128});

	m_textureBarrel = Engine::SubTexture2D::CreateFromCoordinates(m_textureSpriteSheetPacked,{ 8, 0 }, { 128,128 });


	m_orthoCameraController.SetZoomLevel(10.0f);

    Engine::FramebufferSpecification framebufferSpecs;

    framebufferSpecs.Height = 720;
    framebufferSpecs.Width = 1280;
    m_framebuffer = Engine::Framebuffer::Create(framebufferSpecs);

}

void Sandbox2D::OnDetach()
{
	EE_PROFILE_FUNCTION();

}

void Sandbox2D::OnImGuiRender()
{
	EE_PROFILE_FUNCTION();


    
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
        m_framebuffer->Bind();
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
		Engine::Renderer2D::EndScene();
        m_framebuffer->Unbind();

    }

	/*
	Engine::Renderer2D::BeginScene(m_orthoCameraController.GetCamera());


	for (uint32_t y = 0; y  < m_mapHeight; y ++)
	{
		// TOOD ? : combin the vertices and draw as one mesh
		for (uint32_t x = 0; x < m_mapWidth; x++)
		{
			char tileType = s_mapTiles[x + y * m_mapWidth];
			Engine::Ref<Engine::SubTexture2D> texture;
			if (m_textureMap.find(tileType) != m_textureMap.end())
			{
				texture = m_textureMap[tileType];
			}
			else
			{
				texture = m_textureBarrel;

			}
			Engine::Renderer2D::DrawQuad({ x - m_mapWidth / 2.0f,m_mapHeight- y - m_mapHeight / 2.0f, 0.1f }, { 1.0f, 1.0f, }, texture);

		}
	}

	Engine::Renderer2D::EndScene();
	*/

}

void Sandbox2D::OnEvent(Engine::Event& event)
{
	m_orthoCameraController.OnEvent(event);

}


