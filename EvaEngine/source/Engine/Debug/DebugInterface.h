#pragma once
#include <Engine/Map/TextureStreaming/TextureStreamingSystem.h>
#include "entt.hpp"

namespace Engine {

    class DebugInterface
    {
    public:
        static void SetTextureStreamingSystem(TextureStreamingSystem* system) { s_textureSystem = system; }
        static void ResetAllTextures(entt::registry& gameRegistry);

        static void DebugDrawChunkOutlines(entt::registry& gameRegistr);
        

    private:
        static inline TextureStreamingSystem* s_textureSystem = nullptr;
    };
}

