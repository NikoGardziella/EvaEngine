#include "Engine.h"
#include "Engine/Core/Layer.h"

class ExampleLayer : public Engine::Layer
{
	public: ExampleLayer() :
		Layer("Example")
	{

	}

	void OnUpdate() override
	{
		EE_INFO("example layer update");
	}

	void OnEvent(Engine::Event& event) override
	{
		EE_TRACE("{0}", event.ToString());
	}

};

class Sandbox : public Engine::Application
{
public:
	Sandbox()
	{
		PushLayer(new ExampleLayer());
		PushOverLay(new Engine::ImGuiLayer());
	}
	~Sandbox()
	{

	}

};


Engine::Application* Engine::CreateApplication()
{
	return new Sandbox;
} 