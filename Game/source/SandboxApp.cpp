#include "Engine.h"

#include "Engine/AssetManager/AssetManager.h"

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
	std::array<glm::vec2, 2> viewportBounds = { glm::vec2(0.0f, 0.0f), glm::vec2(1.0f, 1.0f) };
	game->GetActiveGameScene()->OnViewportResize(app->GetWindow().GetWidth(), app->GetWindow().GetHeight(), viewportBounds);
	
	Engine::AssetManager::CreateTileAtlas();
	game->LoadGameAssets();
	game->OnGameStart();
	app->PushLayer(game);

	return app;
	
}
//#endif

