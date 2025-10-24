#pragma once
#include <vulkan/vulkan.h>
#include <cstdint>
#include <vector>
#include <algorithm>

#if __has_include(<bit>)
#include <bit>
#define DBR_HAS_COUNTR_ZERO 1
#else
#define DBR_HAS_COUNTR_ZERO 0
#endif
#include <glm/glm.hpp>

namespace Engine {



    struct DirtyTileRuntime {
        std::vector<uint8_t> aliveBits; 
        glm::vec2            topLeft;
    };

    class DeltaBitReader {
    public:
        // Must match your shader constants
        static constexpr uint32_t TILE_W = 128;
        static constexpr uint32_t TILE_H = 256;
        static constexpr uint32_t WORD_BITS = 32;
        static constexpr uint32_t PIXELS_PER_TILE = TILE_W * TILE_H;
        static constexpr uint32_t WORDS_PER_TILE = (PIXELS_PER_TILE + WORD_BITS - 1) / 32;

        DeltaBitReader() = default;

        // Map/unmap the SSBO memory (binding=4). numTiles = slot count (e.g., ACTIVE_SLOTS).
        bool Map(VkDevice device, VkDeviceMemory memory, uint32_t numTiles);
        void Unmap();

        // Pointer to the WORDS_PER_TILE words for a tile (slot)
        inline const uint32_t* Slice(uint32_t tileIdx) const {
            return m_base + tileIdx * WORDS_PER_TILE;
        }

        // Quick check: any bit set in this slice?
        bool Any(uint32_t tileIdx) const;

        // Iterate set pixels that became zero this frame: calls cb(px, py) for each
        template <class F>
        inline void ForEachSetPixel(uint32_t tileIdx, F&& cb) const {
            const uint32_t* s = Slice(tileIdx);
            for (uint32_t w = 0; w < WORDS_PER_TILE; ++w) {
                uint32_t word = s[w];
                while (word) {
                    uint32_t b;
#if DBR_HAS_COUNTR_ZERO && defined(__cpp_lib_bitops)
                    b = std::countr_zero(word);
#elif defined(__GNUG__) || defined(__clang__)
                    b = __builtin_ctz(word);
#elif defined(_MSC_VER)
                    unsigned long tz;
                    _BitScanForward(&tz, word);
                    b = (uint32_t)tz;
#else
                    // Portable fallback
                    b = 0;
                    while (b < 32 && ((word & (1u << b)) == 0)) ++b;
#endif
                    word &= (word - 1);               // clear lowest set bit
                    uint32_t idx = (w << 5) + b;      // w*32 + b
                    if (idx >= PIXELS_PER_TILE) break;
                    uint32_t px = idx % TILE_W;
                    uint32_t py = idx / TILE_W;
                    cb(px, py);
                }
            }
        }

        // Merge slice -> alive bitset (clear bits for pixels that died)
        void MergeIntoAlive(uint32_t tileIdx, DirtyTileRuntime& t) const;

        // Build one dirty-cell bbox [minCell, maxCellHalfOpen] for this slice (optional)
        bool DirtyCellsBBox(uint32_t tileIdx,
            uint32_t DIRTY_CELLS_PER_TILE,
            uint32_t& outMinX, uint32_t& outMinY,
            uint32_t& outMaxX, uint32_t& outMaxY) const;

        inline uint32_t NumTiles() const { return m_numTiles; }

    private:
        static inline void EnsureAliveAllocated(DirtyTileRuntime& t) {
            const size_t need = (PIXELS_PER_TILE + 7) / 8;
            if (t.aliveBits.size() != need) t.aliveBits.assign(need, 0xFF);
        }

    private:
        VkDevice        m_device = VK_NULL_HANDLE;
        VkDeviceMemory  m_memory = VK_NULL_HANDLE;
        void* m_mapped = nullptr;
        const uint32_t* m_base = nullptr;
        uint32_t        m_numTiles = 0;
    };


}
