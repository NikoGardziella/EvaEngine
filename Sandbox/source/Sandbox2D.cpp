#include "Sandbox2D.h"
#include <Engine/Debug/Instrumentor.h>
#include <Engine/AssetManager/AssetManager.h>
#include <Engine/Scene/SceneSerializer.h>

#include <imgui/imgui.h>
#include <glm/ext/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

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

Sandbox2D::Sandbox2D(std::string scene)
	: Layer ("Sandbox2D"),
	m_orthoCameraController(1280.0f / 720.0f, true),
	m_activeSceneName(scene)
{
	m_activeScene = std::make_shared<Engine::Scene>();

}

void Sandbox2D::OnAttach()
{
	EE_PROFILE_FUNCTION();

	m_checkerBoardTexture = Engine::Texture2D::Create(Engine::AssetManager::GetAssetPath("textures/chess_board.png").string());
	m_textureSpriteSheetPacked = Engine::Texture2D::Create(Engine::AssetManager::GetAssetPath("textures/game/RPGpack_sheet_2X.png").string());

	m_mapWidth = s_mapWidth;
	m_mapHeight = strlen(s_mapTiles) / s_mapWidth;
	m_textureMap['D'] = Engine::SubTexture2D::CreateFromCoordinates(m_textureSpriteSheetPacked, {6, 11}, {128,128});
	m_textureMap['W'] = Engine::SubTexture2D::CreateFromCoordinates(m_textureSpriteSheetPacked, {11, 11}, {128,128});

	m_textureBarrel = Engine::SubTexture2D::CreateFromCoordinates(m_textureSpriteSheetPacked,{ 8, 0 }, { 128,128 });


	m_orthoCameraController.SetZoomLevel(10.0f);

    Engine::FramebufferSpecification framebufferSpecs;
	framebufferSpecs.Attachments = { Engine::FramebufferTextureFormat::RGBA8,Engine::FramebufferTextureFormat::RED_INTEGER, Engine::FramebufferTextureFormat::Depth };

    framebufferSpecs.Height = 720;
    framebufferSpecs.Width = 1280;
    m_framebuffer = Engine::Framebuffer::Create(framebufferSpecs);


	/*
	m_cameraEntity = m_activeScene->CreateEntity("camera");
	m_cameraEntity.AddComponent<Engine::CameraComponent>();
	m_cameraEntity.AddComponent<Engine::TransformComponent>();

	*/
	
	/*
	Engine::SceneSerializer serializer(m_activeScene);
	std::string scenePath = Engine::AssetManager::GetScenePath(m_activeSceneName).string();
	if (!serializer.Deserialize(scenePath))
	{
		EE_CORE_ERROR("Failed to load scene at: {}", scenePath);
	}
	*/
	
	
	m_squareEntity = m_activeScene->CreateEntity("square");
	m_squareEntity.AddComponent<Engine::TransformComponent>();
	m_squareEntity.AddComponent<Engine::SpriteRendererComponent>();
	m_squareEntity = m_activeScene->CreateEntity("square1");
	m_squareEntity.AddComponent<Engine::TransformComponent>();
	m_squareEntity.AddComponent<Engine::SpriteRendererComponent>();
	m_squareEntity = m_activeScene->CreateEntity("square2");
	Engine::TransformComponent& transformComp = m_squareEntity.AddComponent<Engine::TransformComponent>();
	transformComp.Translation += glm::vec3(0.0f, 0.0f, -10.0f);
	m_squareEntity.AddComponent<Engine::SpriteRendererComponent>();

	

	
	//m_activeScene->OnRunTimeStart();

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
		m_framebuffer->ClearColorAttachment(1, -1);
		

		m_activeScene->OnUpdateRuntime(timestep, m_isPlaying);
		


		/*
		static float rotation = 0.0f;
		rotation += timestep * 20.0f;

		EE_PROFILE_SCOPE("render draw");
		Engine::Renderer2D::BeginScene(m_orthoCameraController.GetCamera());
		Engine::Renderer2D::DrawQuad({ 0.5f, 0.5f }, { 1.0f, 1.0f, }, { 0.8f, 0.3f, 0.3f, 1.0f });
		Engine::Renderer2D::DrawQuad({ 1.5f, 1.5f }, { 1.0f, 1.0f, }, { 0.2f, 0.3f, 0.9f, 1.0f });
		Engine::Renderer2D::DrawRotatedQuad({ 1.0f,1.0f }, { 1.0f, 1.0f, }, rotation, { 0.1f, 0.8f, 0.4f, 1.0f });
		Engine::Renderer2D::DrawRotatedQuad({ 0.0f,0.0f, -0.1f }, { 10.f, 10.f, }, 45.0f, m_checkerBoardTexture, 10.0f, glm::vec4(1.0f, 0.9f, 0.9f, 1.0f));
		Engine::Renderer2D::EndScene();

		*/
        m_framebuffer->Unbind();

    }

	

}

void Sandbox2D::OnEvent(Engine::Event& event)
{
	m_orthoCameraController.OnEvent(event);

}


