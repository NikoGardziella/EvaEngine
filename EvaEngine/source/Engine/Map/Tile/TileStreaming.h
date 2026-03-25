#pragma once
#include <cstdint>
#include <string>
#include <vector>
#include <unordered_map>
#include <glm/glm.hpp>
#include <Engine/Renderer/Renderer2D/VulkanRenderer2D.h>
#include <Engine/Map/Grid/TileCollisionMask.h>
#include <Engine/Renderer/Utils/DeltaBitReader.h>
#include <Engine/Map/Utils/IVec2Hasher.h>


namespace Engine {

    class Scene;

    class TileManager;
    enum class TileResidency : uint8_t
    {
        GPU,    // Data on GPU + CPU backup in RAM
        CPU,    // Evicted from GPU, data lives in RAM only
        Disk    // Evicted from RAM, saved to disk cache
    };


    struct TileStreamEntry
    {
        uint64_t    uid         = 0;
        TileResidency residency = TileResidency::Disk;
        uint32_t    slot        = UINT32_MAX;
        glm::vec2   worldCenter = { 0.0f, 0.0f };

        std::vector<uint8_t> colorData;
        std::vector<uint8_t> propsData;

        glm::ivec2 opaqueMin = { 0, 0 };
        glm::ivec2 opaqueMax = { 0, 0 };

        std::string tileName;
        bool dirty = false;
    };

    struct StreamingConfig
    {
        float gpuRadius             = 30.0f;    // tiles within this stay on GPU
        float cpuRadius             = 60.0f;    // tiles within this stay in RAM
        float hysteresis            = 2.0f;     // prevents thrashing at boundaries
        int   maxTransitionsPerFrame = 4;        // budget per frame
        std::string cachePath       = "tile_cache"; // disk cache directory
    };

    
    class TileStreamingSystem
    {
    private:

        struct PriorityEntry 
        {
            uint64_t uid;
            float dist;
        };

        struct PendingEviction
        {
            uint64_t uid;
            uint32_t slot;
        };

    public:
        TileStreamingSystem() = default;
        ~TileStreamingSystem() = default;

        void InitTileStreaming(TileManager* tilemanager, const StreamingConfig& config = {});


        void RegisterTile(uint64_t uid, glm::vec2 worldCenter, const std::vector<uint8_t>& colorData, const std::vector<uint8_t>& propsData,
            const glm::ivec2& opaqueMin, const glm::ivec2& opaqueMax, const std::string& tileName, uint32_t gpuSlot);

        void RegisterTileInitial(uint64_t uid, glm::vec2 worldCenter, const std::vector<uint8_t>& colorData, const std::vector<uint8_t>& propsData, const glm::ivec2& opaqueMin, const glm::ivec2& opaqueMax, const std::string& tileName, uint32_t gpuSlot, TileResidency initialResidency);

        void PrimeInitialGPUResidency(Scene* scene, glm::vec2 focusPos);

        void UpdateStreaming(Scene* scene, glm::vec2 playerPos);

        TileResidency GetResidency(uint64_t uid) const;

        uint32_t GetSlot(uint64_t uid) const;


        void ForceAllToGPU(Scene* scene);

        void FlushAllToDisk();

        /// Stats
        uint32_t GetGPUResidentCount() const;
        uint32_t GetCPUResidentCount() const;
        uint32_t GetDiskResidentCount() const;

        void ReadbackFromGPU(TileStreamEntry& entry);
        void SyncDirtyFlags(const std::vector<DirtyTileRuntime>& dirtyTiles);
        void Shutdown();

    private:
        // State transitions
        void EvictFromGPU(TileStreamEntry& entry);
        void UploadToGPU(TileStreamEntry& entry, Scene* scene);
        void FlushToDisk(TileStreamEntry& entry);
        void LoadFromDisk(TileStreamEntry& entry);

        void ReconstructFromAtlas(TileStreamEntry& entry);

        void UpdateTileSlot(Scene* scene, uint64_t uid, uint32_t newSlot);

        void InvalidateTileSlot(Scene* scene, uint64_t uid);

        std::string GetDiskPath(uint64_t uid) const;

        void EnsureCacheDirectory() const;

    private:
        StreamingConfig                                 m_config;
        std::unordered_map<uint64_t, TileStreamEntry>   m_entries;
        std::unordered_map<uint32_t, uint64_t> m_slotToUID;
        bool                                            m_initialized = false;

        size_t m_updateCursor = 0;

        TileManager* m_tilemanager;
        std::vector<PriorityEntry> m_priorityList;
        std::vector<PendingEviction> m_pendingEvictions;



        std::unordered_map<glm::ivec2, std::vector<uint64_t>, IVec2Hasher> m_entriesByChunk;

        // Active resident sets only
        std::unordered_set<uint64_t> m_gpuResidentUIDs;
        std::unordered_set<uint64_t> m_cpuResidentUIDs;

        // Optional: avoid updating every tiny movement
        glm::vec2 m_lastUpdatePlayerPos = glm::vec2(std::numeric_limits<float>::max());

    };

}
