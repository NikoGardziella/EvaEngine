#pragma once
#include <glm/ext/vector_float2.hpp>
#include <Engine/Platform/Vulkan/VulkanTexture.h>
#include "Engine/Core/Core.h"
#include <Engine/Core/UUID.h>

#include "entt.hpp"

namespace Engine{

    const int CHUNK_SIZE = 128;
    const int LOAD_RADIUS = 1; // Load a 5×5 chunk area (2 chunks in all directions)
    const int UNLOAD_RADIUS = 1; 

    struct TextureChunk {
        //std::string ID; // Unique ID or path
        UUID ID;
		std::string Name; 
        glm::ivec2 WorldPosition;
        glm::ivec2 ChunkCoords; 
        std::vector<uint8_t> PixelData; 
        bool IsDirty = false; // Mark if pixels were modified
        bool IsLoaded = false;
        Engine::Ref<Engine::VulkanTexture> GPUTexture; 
        uint32_t Width = 0;
        uint32_t Height = 0;
    };


    struct IVec2Hasher
    {
        std::size_t operator()(const glm::ivec2& v) const noexcept
        {
            std::size_t h1 = std::hash<int>{}(v.x);
            std::size_t h2 = std::hash<int>{}(v.y);
            return h1 ^ (h2 << 1); // Combine the hashes
        }
    };



    class TextureStreamingSystem
    {
    public:
        TextureStreamingSystem();
        ~TextureStreamingSystem();

        void TextureStreamingSystem::Update(const glm::vec2& playerPos, entt::registry& gameRegistry);
		void TextureStreamingSystem::UploadToChunkFromTexture(const glm::vec3& worldPosition, UUID ID,
            std::string name, const std::vector<uint8_t>& textureData, uint32_t textureWidth, uint32_t textureHeight);
        
        //debug
        void ResetAllChunks(entt::registry& gameRegistry);
        void DebugDrawChunkOutlines(entt::registry& gameRegistry);
    private:
        void LoadChunkToGPU(TextureChunk& chunk, entt::registry& gameRegistry);

        void UnloadChunkFromGPU(TextureChunk& chunk, entt::registry& gameRegistry);

        Ref<VulkanTexture> CreateTextureFromData(const uint8_t* pixelData, int width, int height);

        std::unordered_map<UUID, TextureChunk> m_chunkMap;

    };


}

