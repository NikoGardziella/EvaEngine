#pragma once
#include <Engine/Debug/Instrumentor.h>
#include "Engine/AssetManager/AssetManager.h"
#include "Engine/Core/Application.h"

//#include <Engine/Renderer/ShaderCompiler.h>
#include "Sandbox2D.h"
//#include "PixelGame.h"
#ifdef EE_PLATFORM_WINDOWS





int main(int argc, char** argv)
{
	Engine::Log::Init();
	const int assetFolderSearchDepth = 5;
	Engine::AssetManager::Initialize(assetFolderSearchDepth);
	

	EE_PROFILE_BEGIN_SESSION("Startup", "EvaEngineProfile-startup.json");
	Engine::Application* app = nullptr;
	if(argc > 1 && (std::string(argv[1]) == "editor" || std::string(argv[1]) == "Editor"))
	{
		app = Engine::CreateEditorApplication();
	}
	else
	{
		app = Engine::CreateApplication(); // Sandbox
		
	}
	EE_PROFILE_END_SESSION();



	EE_PROFILE_BEGIN_SESSION("Runtimep", "EvaEngineProfile-runtime.json");
	app->Run();
	EE_PROFILE_END_SESSION();

	EE_PROFILE_BEGIN_SESSION("Shutdown", "EvaEngineProfile-shutdown.json");
	delete app;
	EE_PROFILE_END_SESSION();


}
#endif 
