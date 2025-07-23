#pragma once
#include <Engine/Platform/Vulkan/VulkanTexture.h>
#include "Engine/Core/Core.h"
#include <Engine/Core/UUID.h>

#include "entt.hpp"
#include <Engine/Map/Utils/IVec2Hasher.h>

namespace Engine{

    const uint32_t CHUNK_SIZE = 32;
    const int LOAD_RADIUS = 2; // Load a 3×3 chunk area (1 chunks in all directions)
    const int UNLOAD_RADIUS = 2; 

    struct TextureChunk {
        //std::string ID; // Unique ID or path
        UUID ID;
		std::string Name;
		std::string AssetName;
        glm::ivec2 WorldPosition;
        glm::ivec2 ChunkCoords; 
        std::vector<uint8_t> PixelData; 
        bool IsDirty = false; // Mark if pixels were modified
        bool IsLoaded = false;
        Engine::Ref<Engine::VulkanTexture> GPUTexture; 
        uint32_t Width = 0;
        uint32_t Height = 0;
        uint32_t TextureCount = 0;
    };

   




    class Scene;
    class TextureStreamingSystem
    {
    public:
        TextureStreamingSystem();
        ~TextureStreamingSystem();

        void TextureStreamingSystem::Update(const glm::vec2& playerPos, entt::registry& gameRegistry);
      
		void TextureStreamingSystem::UploadToChunkFromTexture(const glm::vec2& worldPosition, UUID ID,
            std::string name, const std::vector<uint8_t>& textureData, uint32_t textureWidth, uint32_t textureHeight);
        
		std::unordered_map<UUID, TextureChunk>& GetChunkMap() { return m_chunkMap; }


            
        void BakeTilesIntoChunks(entt::registry& registry);
        void AddChunkEntitiesToRegistry(entt::registry& registry);
        void DebugMarkChunks();
        //debug
        void ResetAllChunks(entt::registry& gameRegistry);
        void DebugDrawChunkOutlines(entt::registry& gameRegistry);
    private:
        uint64_t HashCoords(const glm::ivec2& coords);
        void FlipChunkHorizontally(TextureChunk& chunk);
        void FlipChunkVertically(TextureChunk& chunk);
        void LoadChunkToGPU(TextureChunk& chunk, entt::registry& gameRegistry);

        void UnloadChunkFromGPU(TextureChunk& chunk, entt::registry& gameRegistry);

        Ref<VulkanTexture> CreateTextureFromData(const uint8_t* pixelData, int width, int height);

        std::unordered_map<UUID, TextureChunk> m_chunkMap;

    };


}

