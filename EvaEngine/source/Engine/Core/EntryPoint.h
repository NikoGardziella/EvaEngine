#pragma once
#include <Engine/Debug/Instrumentor.h>
//#include <Engine/Renderer/ShaderCompiler.h>


#ifdef EE_PLATFORM_WINDOWS

//defined in application/sandbox
extern Engine::Application* Engine::CreateApplication();

int main(int argc, char** argv)
{
	Engine::Log::Init();

	

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
