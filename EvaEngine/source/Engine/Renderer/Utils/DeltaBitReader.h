#pragma once
#include <vulkan/vulkan.h>
#include <cstdint>
#include <vector>
#include <functional>
#include <glm/glm.hpp>


namespace Engine {



    struct DirtyTileRuntime
    {
        // World-space origin of this tile 
        glm::vec2 topLeft = { 0.0f, 0.0f };
        uint32_t slot = 0;
        // Packed alive bits, 1 bit per pixel, length = WORDS_PER_TILE uint32_t words.
        // Bit 1 == pixel alive, Bit 0 == dead.
        std::vector<uint32_t> aliveWords;

        // Cached counters/flags you might want to keep on CPU.
        uint32_t aliveCount = 0;   
    };

    // Simple reader that memory-maps a HOST_VISIBLE|HOST_COHERENT SSBO with packed alive bits.
    // Bit 1 == alive. Bit 0 == dead.
    class DeltaBitReader {
    public:
        DeltaBitReader() = default;

        // Map the SSBO memory.
        // - memory: VkDeviceMemory bound to the SSBO range
        // - offsetBytes: byte offset inside 'memory' where the bitset starts (usually 0)
        // - numTiles, tileW, tileH: must match the shader
        bool Map(VkDevice device,
            VkDeviceMemory memory,
            uint32_t numTiles,
            uint32_t tileW,
            uint32_t tileH,
            VkDeviceSize offsetBytes = 0);

        void Unmap();

        // Quick checks / access
        uint32_t NumTiles()     const { return m_numTiles; }
        uint32_t TileW()        const { return m_tileW; }
        uint32_t TileH()        const { return m_tileH; }
        uint32_t WordsPerTile() const { return m_wordsPerTile; }

        // Returns true if any alive bit in this tile is set.
        bool Any(uint32_t tileIdx) const;

        bool FindFirstNonZeroTile(uint32_t& outTileIdx) const;

        // Count alive bits in a tile.
        uint32_t CountAlive(uint32_t tileIdx) const;

        // Pointer to packed words for a tile (length = WordsPerTile()).
        const uint32_t* GetTileWords(uint32_t tileIdx) const;

        // Iterate all set pixels (alive == 1) for a tile, calling f(px, py).
        void ForEachSetPixel(uint32_t tileIdx,
            const std::function<void(uint32_t /*px*/, uint32_t /*py*/)>& f) const;

        // Build a coarse AABB in tile pixel space of alive bits.
        // Returns false if empty. On success, outputs inclusive bounds.
        bool AliveBBox(uint32_t tileIdx, int& minx, int& miny, int& maxx, int& maxy) const;

        // Convenience: copy this tile's words into a vector (resizes as needed).
        void CopyTileWords(uint32_t tileIdx, std::vector<uint32_t>& outWords) const;

    private:
        static uint32_t DivCeil(uint32_t a, uint32_t b) { return (a + b - 1u) / b; }
        static uint32_t Popcnt32(uint32_t v);

        // Base pointer to the first word of tile 'tileIdx'
        const uint32_t* Slice(uint32_t tileIdx) const;

    private:
        VkDevice        m_device = VK_NULL_HANDLE;
        VkDeviceMemory  m_memory = VK_NULL_HANDLE;
        void* m_mappedRaw = nullptr;     // base of mapped region (offset already applied)
        const uint32_t* m_base = nullptr;     // typed view of m_mappedRaw

        uint32_t        m_numTiles = 0;
        uint32_t        m_tileW = 0;
        uint32_t        m_tileH = 0;
        uint32_t        m_wordsPerTile = 0;
        VkDeviceSize    m_offsetBytes = 0;
    };

} 
