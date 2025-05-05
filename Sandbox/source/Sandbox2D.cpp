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

	//m_checkerBoardTexture = Engine::Texture2D::Create(Engine::AssetManager::GetAssetPath("textures/chess_board.png").string());
	//m_textureSpriteSheetPacked = Engine::Texture2D::Create(Engine::AssetManager::GetAssetPath("textures/game/RPGpack_sheet_2X.png").string());
	m_texture = Engine::AssetManager::GetTexture("logo");

	//m_mapWidth = s_mapWidth;
	//m_mapHeight = strlen(s_mapTiles) / s_mapWidth;
	//m_textureMap['D'] = Engine::SubTexture2D::CreateFromCoordinates(m_textureSpriteSheetPacked, {6, 11}, {128,128});
	//m_textureMap['W'] = Engine::SubTexture2D::CreateFromCoordinates(m_textureSpriteSheetPacked, {11, 11}, {128,128});

	//m_textureBarrel = Engine::SubTexture2D::CreateFromCoordinates(m_textureSpriteSheetPacked,{ 8, 0 }, { 128,128 });


	m_orthoCameraController.SetZoomLevel(10.0f);

    Engine::FramebufferSpecification framebufferSpecs;
	framebufferSpecs.Attachments = { Engine::FramebufferTextureFormat::RGBA8,Engine::FramebufferTextureFormat::RED_INTEGER, Engine::FramebufferTextureFormat::Depth };

    framebufferSpecs.Height = 720;
    framebufferSpecs.Width = 1280;
    //m_framebuffer = Engine::Framebuffer::Create(framebufferSpecs);

	Engine::SceneSerializer serializer(m_activeScene);
	std::string scenePath = Engine::AssetManager::GetScenePath(m_activeSceneName).string();
	if (!serializer.Deserialize(scenePath))
	{
		EE_CORE_ERROR("Failed to load scene at: {}", scenePath);
	}


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

	/*
	glm::mat4 transform = glm::translate(glm::mat4(1.0f), { 0.0f, 0.0f, 0.0f });
	glm::vec4 color = { 1.0f, 1.0f, 1.0f, 1.0f };

	//const glm::mat4 viewProjection = m_cameraEntity.GetComponent<Engine::CameraComponent>().Camera.GetViewProjection();
	const glm::mat4 viewProjection = glm::translate(glm::mat4(1.0f), { 0.0f, 0.0f, 0.0f });

	Engine::VulkanRenderer2D::BeginScene(viewProjection);
	glm::vec2 position = { 0.9f, 0.7f };
	glm::vec2 size = { 0.2f, 0.3f }; // Width = 2, Height = 3

	glm::mat4 transform1 = glm::translate(glm::mat4(1.0f), { position.x, position.y, 0.0f }) *
		glm::scale(glm::mat4(1.0f), { size.x, size.y, 1.0f });
	glm::vec4 color1 = { 1.0f, 0.2f, 0.2f, 1.0f };

	Engine::VulkanRenderer2D::DrawQuad(transform1, color1);

	Engine::VulkanRenderer2D::DrawTextureQuad(transform,m_texture, 1, color);
	Engine::Renderer::DrawFrame();
	Engine::VulkanRenderer2D::EndScene();

	*/
	// ******** Render ***********
	// statistics
	//Engine::VulkanRenderer2D::ResetStats();
    {
		EE_PROFILE_SCOPE("render pre");
        //m_framebuffer->Bind();
	    //Engine::RenderCommand::SetClearColor({ 0.1f, 0.1f, 0.1f, 1 });
	    //Engine::RenderCommand::Clear();
    }

    {
		//m_framebuffer->ClearColorAttachment(1, -1)
		if (m_isPlaying)
		{
			m_activeScene->OnUpdateRuntime(timestep, m_isPlaying);
		}
		
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
        //m_framebuffer->Unbind();

    }

	

}

void Sandbox2D::OnEvent(Engine::Event& event)
{
	m_orthoCameraController.OnEvent(event);

}

void Sandbox2D::OnGameStart()
{
	/*
	m_squareEntity = m_activeScene->CreateEntity("square");
	m_squareEntity.AddComponent<Engine::TransformComponent>();
	m_squareEntity.AddComponent<Engine::SpriteRendererComponent>();
	m_squareEntity = m_activeScene->CreateEntity("square1");
	m_squareEntity.AddComponent<Engine::TransformComponent>();
	m_squareEntity.AddComponent<Engine::SpriteRendererComponent>();
	*/

	
	m_cameraEntity = m_activeScene->CreateEntity("camera");
	auto& cameraComp = m_cameraEntity.AddComponent<Engine::CameraComponent>();
	cameraComp.FixedAspectRatio = true;
	cameraComp.Camera.SetProjectionType(Engine::SceneCamera::ProjectionType::Perspective);
	cameraComp.Camera.SetPerspectiveFOV(45.0f);
	
	auto& cameraTransformComp = m_cameraEntity.AddComponent<Engine::TransformComponent>();
	cameraTransformComp.Translation += glm::vec3(0.0f, 0.0f, 80.0f);
	
	/*
	m_squareEntity = m_activeScene->CreateEntity("Gamesquare2");
	Engine::TransformComponent& transformComp = m_squareEntity.AddComponent<Engine::TransformComponent>();
	transformComp.Translation += glm::vec3(0.0f, 0.0f, -10.0f);
	m_squareEntity.AddComponent<Engine::SpriteRendererComponent>();
	*/

	

	CreateTestScene();


	m_activeScene->OnRunTimeStart();

}

void Sandbox2D::OnGameStop()
{
	m_activeScene->ClearRegistry();

	Engine::SceneSerializer serializer(m_activeScene);
	serializer.Deserialize(Engine::AssetManager::GetAssetPath(m_activeSceneName).string());
}

void Sandbox2D::CreateTestScene()
{

	uint32_t boxCount = 0;
	uint32_t circleCount = 0;
	for (int i = 0; i < 500; i++) // Create 10 entities
	{
		// Generate a random position
		glm::vec3 position = {
			(rand() % 40 - 20) * 1.0f,  // X: Random between -20 and 20
			(rand() % 40 - 20) * 1.0f,  // Y: Random between -20 and 20
			0.0f                      // Z: Fixed depth
		};


		// Generate a random color
		glm::vec4 color = {
			static_cast<float>(rand()) / RAND_MAX, // R
			static_cast<float>(rand()) / RAND_MAX, // G
			static_cast<float>(rand()) / RAND_MAX, // B
			1.0f  // Alpha (fully opaque)
		};

		// Randomly choose between box or circle
		bool isBox = 2 % 2 == 0;

		// Create entity with a unique name
		Engine::Entity entity = m_activeScene->CreateEntity(isBox ? "GameBox_" + std::to_string(i) : "GameCircle_" + std::to_string(i));

		// Add Transform Component
		Engine::TransformComponent& transformComp = entity.AddComponent<Engine::TransformComponent>();
		transformComp.Translation = position;

		// Add Sprite Renderer Component

		// Add Rigidbody Component
		Engine::RigidBody2DComponent& rbComp = entity.AddComponent<Engine::RigidBody2DComponent>();
		rbComp.Type = Engine::RigidBody2DComponent::BodyType::Dynamic; // Set to Dynamic

		// Add the correct collider
		if (isBox)
		{
			boxCount++;
			// Add Box Collider
			Engine::BoxCollider2DComponent& colliderComp = entity.AddComponent<Engine::BoxCollider2DComponent>();
			Engine::SpriteRendererComponent& spriteComp = entity.AddComponent<Engine::SpriteRendererComponent>();
			spriteComp.Color = color;

		}
		else
		{
			circleCount++;
			// Add Circle Collider
			Engine::CircleCollider2DComponent& colliderComp = entity.AddComponent<Engine::CircleCollider2DComponent>();
			Engine::CircleRendererComponent& spriteComp = entity.AddComponent<Engine::CircleRendererComponent>();
			spriteComp.Color = color;
			colliderComp.Radius = 0.5f; // Standard circle radius
		}

	}

	EE_INFO("box count: {}   |   circle count:  {}", boxCount, circleCount);

}


