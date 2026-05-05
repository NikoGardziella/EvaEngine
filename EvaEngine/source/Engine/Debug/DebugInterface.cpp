#include "pch.h"
#include "DebugInterface.h"


namespace Engine {

	//TextureStreamingSystem* DebugInterface::s_textureSystem = nullptr;
	glm::vec4 DebugInterface::s_debugValues;

	void Engine::DebugInterface::ResetAllTextures(Scene* scene)
	{
		if (s_textureSystem)
			s_textureSystem->ResetAllChunks(scene);
	}
	void Engine::DebugInterface::DebugDrawChunkOutlines(Scene* scene)
	{
		if (s_textureSystem)
			s_textureSystem->DebugDrawChunkOutlines(scene);
	}
}
