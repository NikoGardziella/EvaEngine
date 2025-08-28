#pragma once
#include <Engine/Map/TextureStreaming/TextureStreamingSystem.h>


namespace Engine {

    class DebugInterface
    {
    public:
        static void SetTextureStreamingSystem(TextureStreamingSystem* system) { s_textureSystem = system; }
        static void ResetAllTextures(Scene* scene);

        static void DebugDrawChunkOutlines(Scene* scene);
        

    private:
        static inline TextureStreamingSystem* s_textureSystem = nullptr;
    };
}

