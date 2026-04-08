#pragma once
#include <Engine/Platform/Vulkan/VulkanTexture.h>

#include <Engine/Core/UUID.h>


namespace Engine{

    const int LOAD_RADIUS = 1; // Load a 3×3 chunk area (1 chunks in all directions)
    const int UNLOAD_RADIUS = 1; 
    const int CHUNK_GRID_WIDTH = LOAD_RADIUS * 2 + 1;
    const int CHUNK_GRID_SIZE = CHUNK_GRID_WIDTH * CHUNK_GRID_WIDTH;
    const int DISK_RADIUS = UNLOAD_RADIUS + 3;

    const int MAX_TRANSITIONS = 2;





    class GridMap;
    class Scene;
    class TextureStreamingSystem
    {

    private:

        enum class ChunkResidency : uint8_t
        {
            GPU,    // on GPU + CPU
            CPU,    // CPU only
            Disk    // flushed to disk
        };


        struct TerrainBakeItem
        {
            glm::vec2 worldTilePos;
            UUID id;
            std::string name;
            const TileInfo* tile = nullptr;
            std::vector<uint8_t> pixelData;
            uint32_t w = 0;
            uint32_t h = 0;
        };

        struct TextureChunk 
        {
            UUID ID;
            std::string Name;
            std::string AssetName;
            glm::ivec2 ChunkCoords;
            std::vector<uint8_t> PixelData;
            std::vector<uint8_t> PropertiesData; // RGBA8UI: R=health, G=height, B=mask, A=flags)
            std::vector<uint8_t> TerrainData;
            bool IsDirty = false; // Mark if pixels were modified
            bool IsLoaded = false;
            Engine::Ref<Engine::VulkanTexture> GPUTexture;
            Engine::Ref<Engine::VulkanTexture> PropertiesTexture;
            Engine::Ref<Engine::VulkanTexture> TerrainTexture;

            uint32_t Width = 0;
            uint32_t Height = 0;
            uint32_t TextureCount = 0;
            uint32_t RenderSlot = 0;
            ChunkResidency Residency = ChunkResidency::CPU;
            bool IsDirtyPixels = false;

        };


        

    public:
        TextureStreamingSystem();
        ~TextureStreamingSystem();

        void TextureStreamingSystem::Update(const glm::vec2& playerPos, Scene* scene);

        void UnloadAllChunks(Scene* scene);


     
		void TextureStreamingSystem::UploadToChunkFromTexture(const glm::vec2& worldPosition, UUID ID,
            const std::string& name, const std::vector<uint8_t>& textureData, const std::vector<uint8_t>& propertiesData, uint32_t textureWidth, uint32_t textureHeight);

        void UploadTerrainToChunkFromTexture(glm::vec2& worldPosition, UUID ID, std::string name, const std::vector<uint8_t>& textureData, uint32_t textureWidth, uint32_t textureHeight);
        
		std::unordered_map<UUID, TextureChunk>& GetChunkMap() { return m_chunkMap; }

        void SetGridMap(Ref<GridMap>& gridmap) { m_gridMap = gridmap; }
            
        void BakeTilesIntoChunks(Scene* scene);
        void FillTerrainGaps(TextureChunk& chunk);
        void SortIsoTilesByY(Scene* scene);
        void AddChunkEntitiesToRegistry(Scene* scene);
        void FlushChunkToDisk(TextureChunk& chunk);
        void LoadChunkFromDisk(TextureChunk& chunk, Scene* scene);
        std::string GetChunkDiskPath(const glm::ivec2& coords) const;
        void ReconstructChunkFromAtlas(TextureChunk& chunk, Scene* scene);
        void DebugMarkChunks();
        //debug
        void ResetAllChunks(Scene* scene);
        void DebugDrawChunkOutlines(Scene* scene);
        
        
        bool DebugWriteTGA32(const char* path, int w, int h, const std::vector<uint8_t>& rgba);
        bool DebugWritePPM(const char* path, int w, int h, const std::vector<uint8_t>& rgba);
        void DumpRGBA(const std::string& filename, int w, int h, const std::vector<uint8_t>& rgba);
    private:
        void SortChunksRowMajor(Scene* scene);
        uint64_t HashCoords(const glm::ivec2& coords);
        void FlipChunkHorizontally(TextureChunk& chunk);
        void FlipChunkVertically(TextureChunk& chunk);
        void LoadChunkToGPU(TextureChunk& chunk, Scene* scene);

        void UnloadChunkFromGPU(TextureChunk& chunk, Scene* scene);


        std::unordered_map<UUID, TextureChunk> m_chunkMap;
        Ref<GridMap> m_gridMap;

        std::string m_chunkCachePath = "chunk_cache";

    };


}

