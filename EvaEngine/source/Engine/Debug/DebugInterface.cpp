#include "pch.h"
#include "DebugInterface.h"


namespace Engine {

	//TextureStreamingSystem* DebugInterface::s_textureSystem = nullptr;

	void Engine::DebugInterface::ResetAllTextures(entt::registry& gameRegistry)
	{
		if (s_textureSystem)
			s_textureSystem->ResetAllChunks(gameRegistry);
	}
}
