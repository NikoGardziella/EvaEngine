#pragma once
#include <Engine/Debug/Instrumentor.h>
#include "Engine/AssetManager/AssetManager.h"

//#include <Engine/Renderer/ShaderCompiler.h>
//#include "Sandbox2D.h"

#ifdef EE_PLATFORM_WINDOWS

//defined in application/sandbox
//extern Engine::Application* Engine::CreateApplication();
//extern Engine::Application* Engine::CreateGameApplication();


int main(int argc, char** argv)
{
	Engine::Log::Init();
	const int assetFolderSearchDepth = 5;
	Engine::AssetManager::Initialize(assetFolderSearchDepth);
	

	EE_PROFILE_BEGIN_SESSION("Startup", "EvaEngineProfile-startup.json");
	auto app = Engine::CreateApplication();	
	EE_PROFILE_END_SESSION();

	EE_PROFILE_BEGIN_SESSION("Runtimep", "EvaEngineProfile-runtime.json");
	app->Run();
	EE_PROFILE_END_SESSION();

	EE_PROFILE_BEGIN_SESSION("Shutdown", "EvaEngineProfile-shutdown.json");
	delete app;
	EE_PROFILE_END_SESSION();


}
#endif 
