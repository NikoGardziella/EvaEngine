#pragma once
#include <Engine/Platform/Vulkan/VulkanTexture.h>
#include "Engine/Core/Core.h"
#include <Engine/Core/UUID.h>

#include "entt.hpp"
#include <Engine/Map/Utils/IVec2Hasher.h>
#include "Engine/Map/Grid/GridMap.h"

namespace Engine{

    const int LOAD_RADIUS = 1; // Load a 3×3 chunk area (1 chunks in all directions)
    const int UNLOAD_RADIUS = 1; 
    const int CHUNK_GRID_WIDTH = LOAD_RADIUS * 2 + 1;
    const int CHUNK_GRID_SIZE = CHUNK_GRID_WIDTH * CHUNK_GRID_WIDTH;

    struct TextureChunk {
        UUID ID;
		std::string Name;
		std::string AssetName;
        glm::ivec2 ChunkCoords; 
        std::vector<uint8_t> PixelData; 
        std::vector<uint8_t> HealthData;
        std::vector<uint8_t> TerrainData;
        bool IsDirty = false; // Mark if pixels were modified
        bool IsLoaded = false;
        Engine::Ref<Engine::VulkanTexture> GPUTexture; 
        Engine::Ref<Engine::VulkanTexture> HealthTexture; 
        Engine::Ref<Engine::VulkanTexture> TerrainTexture; 

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
            std::string name, const std::vector<uint8_t>& textureData, const std::vector<uint8_t>& healthData, uint32_t textureWidth, uint32_t textureHeight);

        void UploadTerrainToChunkFromTexture(const glm::vec2& worldPosition, UUID ID, std::string name, const std::vector<uint8_t>& textureData, uint32_t textureWidth, uint32_t textureHeight);
        
		std::unordered_map<UUID, TextureChunk>& GetChunkMap() { return m_chunkMap; }

        void SetGridMap(Ref<GridMap>& gridmap) { m_gridMap = gridmap; }
            
        void BakeTilesIntoChunks(entt::registry& registry);
        void AddChunkEntitiesToRegistry(entt::registry& registry);
        void DebugMarkChunks();
        //debug
        void ResetAllChunks(entt::registry& gameRegistry);
        void DebugDrawChunkOutlines(entt::registry& gameRegistry);
    private:
        void SortChunksRowMajor(entt::registry& reg);
        uint64_t HashCoords(const glm::ivec2& coords);
        void FlipChunkHorizontally(TextureChunk& chunk);
        void FlipChunkVertically(TextureChunk& chunk);
        void LoadChunkToGPU(TextureChunk& chunk, entt::registry& gameRegistry);

        void UnloadChunkFromGPU(TextureChunk& chunk, entt::registry& gameRegistry);


        std::unordered_map<UUID, TextureChunk> m_chunkMap;
        Ref<GridMap> m_gridMap;

        

    };


}

