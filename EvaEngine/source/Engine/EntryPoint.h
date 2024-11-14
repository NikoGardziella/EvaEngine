#pragma once

#ifdef EE_PLATFORM_WINDOWS

//defined in application/sandbox
extern Engine::Application* Engine::CreateApplication();

int main(int argc, char** argv)
{
	auto app = Engine::CreateApplication();
	app->Run();

	delete app;
}
#endif 
