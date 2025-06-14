#include "Engine.h"
#include "Engine/Core/Layer.h"
#include "Engine/Core/EntryPoint.h"
#include <Engine/Scene/SceneSerializer.h>
#include "Engine/AssetManager/AssetManager.h"
#include <Engine/Platform/OpenGl/OpenGLShader.h>

#include "PixelGame.h"

class GameApp : public Engine::Application
{
public:
	GameApp()
	{
		

	}
	~GameApp()
	{

	}

	void InitApp()
	{

	}

};



//#ifdef GAME_BUILD

/*
Engine::Application* Engine::CreateEditorApplication()
{

	EE_CORE_WARN("This is a test");
	return nullptr;
}
*/

Engine::Application* Engine::CreateApplication()
{
	auto app = new GameApp;
	
	auto game = new PixelGame("currentScene");
	game->SetViewportSize(app->GetWindow().GetWidth(), app->GetWindow().GetHeight());
	game->LoadGameAssets();
	game->OnGameStart();
	app->PushLayer(game);
	

	return app;
	
}
//#endif

