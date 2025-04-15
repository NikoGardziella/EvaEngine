#include "Engine.h"
#include "Engine/Core/Layer.h"
#include "Engine/Core/EntryPoint.h"
#include <Engine/Scene/SceneSerializer.h>
#include "Engine/AssetManager/AssetManager.h"
#include <Engine/Platform/OpenGl/OpenGLShader.h>

#include <imgui/imgui.h>
#include <glm/ext/matrix_transform.hpp>

#include <glm/gtc/type_ptr.hpp>

#include "Sandbox2D.h"

//#include "Engine/Core/EntryPoint.h"

class ExampleGameLayer : public Engine::Layer
{
	public: ExampleGameLayer() :
		Layer("Example")
		
	{

		

	}

	void OnAttach()
	{
		Engine::FramebufferSpecification framebufferSpecs;

		framebufferSpecs.Attachments = { Engine::FramebufferTextureFormat::RGBA8 , Engine::FramebufferTextureFormat::RED_INTEGER, Engine::FramebufferTextureFormat::Depth };
		framebufferSpecs.Height = (uint32_t)m_viewportSize.y;
		framebufferSpecs.Width = (uint32_t)m_viewportSize.x;
		//m_framebuffer = Engine::Framebuffer::Create(framebufferSpecs);



		m_activeScene = std::make_shared<Engine::Scene>();
		Engine::SceneSerializer serializer(m_activeScene);
		std::string scenePath = Engine::AssetManager::GetAssetPath("scenes/physics2D.EE").string();
		if (!serializer.Deserialize(scenePath))
		{
			EE_CORE_ERROR("Failed to load scene at: {}", scenePath);
		}
		else
		{
			EE_CORE_INFO("Scene loaded successfully!");
		}

		//m_framebuffer->Resize(static_cast<uint32_t>(m_viewportSize.x), static_cast<uint32_t>(m_viewportSize.y));

		m_activeScene->OnRunTimeStart();
		//m_activeScene->OnViewportResize((uint32_t)m_viewportSize.x, (uint32_t)m_viewportSize.y);
		//m_viewportRenderTexture = Engine::Texture2D::Create(m_viewportSize.x, m_viewportSize.y);

	}

	void OnUpdate(Engine::Timestep timestep) override
	{
		
		/*
		Engine::FramebufferSpecification spec = m_framebuffer->GetSpecification();
		if (m_viewportSize.x > 0.0f && m_viewportSize.y > 0.0f &&
			(spec.Width != static_cast<uint32_t>(m_viewportSize.x) ||
				spec.Height != static_cast<uint32_t>(m_viewportSize.y)))
		{
			m_framebuffer->Resize(static_cast<uint32_t>(m_viewportSize.x), static_cast<uint32_t>(m_viewportSize.y));
			m_activeScene->OnViewportResize(static_cast<uint32_t>(m_viewportSize.x), static_cast<uint32_t>(m_viewportSize.y));
		}
		

		Engine::RenderCommand::Clear();
		Engine::RenderCommand::SetClearColor({ 0.0f, 0, 0.2f, 1.0f });
		
		m_framebuffer->ClearColorAttachment(1, -1);
		m_framebuffer->Bind();
		m_framebuffer->Unbind();
		*/
		
		//m_activeScene->OnUpdateRuntime(timestep);

		

	}

	

	void OnEvent(Engine::Event& event) override
	{
		//Engine::EventDispatcher dispatcher(event);
		//dispatcher.Dispatch<Engine::KeyPressedEvent>(EE_BIND_EVENT_FN(ExampleLayer::OnKeyPressedEvent));

	

	}

	bool OnKeyPressedEvent(Engine::KeyPressedEvent& event)
	{
		
		return false;
	}

	

	virtual void OnImGuiRender() override
	{
	
		
	}




private:

	Engine::Ref<Engine::Framebuffer> m_framebuffer;
	Engine::Ref<Engine::Scene> m_activeScene;
	glm::vec2 m_viewportSize = { 720.0f, 1280.0f };
	Engine::EditorCamera m_editorCamera;
	Engine::Ref<Engine::Texture2D> m_viewportRenderTexture;
};

class Sandbox : public Engine::Application
{
public:
	Sandbox()
	{
		//PushLayer(new ExampleGameLayer());
		PushLayer(new Sandbox2D());
	}
	~Sandbox()
	{

	}


};



//#ifdef GAME_BUILD

Engine::Application* Engine::CreateApplication()
{
	return new Sandbox(); // Only for sandbox builds
}
//#endif

