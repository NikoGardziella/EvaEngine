#pragma once

#ifdef EE_PLATFORM_WINDOWS

//defined in application/sandbox
extern Engine::Application* Engine::CreateApplication();

int main(int argc, char** argv)
{
	Engine::Log::Init();
	EE_CORE_INFO("Core logger initialized");

	EE_TRACE("Client logger initialized number= {0}", "hello");


	auto app = Engine::CreateApplication();
	app->Run();

	delete app;
}
#endif 
