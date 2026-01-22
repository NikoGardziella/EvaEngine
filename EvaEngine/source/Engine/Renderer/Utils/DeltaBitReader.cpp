#include "pch.h"
#include "DeltaBitReader.h"
#include "DeltaBitReader.h"
#include <cstring>
#include <Engine/Debug/Instrumentor.h>
#include <Engine/Core/Assert.h>

namespace Engine {


#include <cstdint>
#if defined(_MSC_VER)
#include <intrin.h>     // _BitScanForward, __popcnt
#include <immintrin.h>  // _tzcnt_u32 (BMI1) if available
#endif

    // Count trailing zeros for non-zero 32-bit value.
    static inline uint32_t ctz32(uint32_t x) {
        // x must be non-zero; our callers only pass non-zero words.
#if defined(_MSC_VER) && !defined(__clang__)
    // Prefer BMI1 tzcnt if available; otherwise use BitScanForward
#if defined(__AVX2__) || defined(__BMI__) || defined(__BMI2__)
        return _tzcnt_u32(x);
#else
        unsigned long idx = 0;
        _BitScanForward(&idx, x);  // defined for x != 0
        return static_cast<uint32_t>(idx);
#endif
#elif defined(__clang__) || defined(__GNUC__)
        return __builtin_ctz(x);       // defined for x != 0
#else
    // Portable fallback
        uint32_t n = 0;
        while ((x & 1u) == 0u) { x >>= 1; ++n; }
        return n;
#endif
    }




    uint32_t DeltaBitReader::Popcnt32(uint32_t v) {
#if defined(_MSC_VER)
        return __popcnt(v);
#else
        return __builtin_popcount(v);
#endif
    }

    bool DeltaBitReader::Map(VkDevice device,
        VkDeviceMemory memory,
        uint32_t numTiles,
        uint32_t tileW,
        uint32_t tileH,
        VkDeviceSize offsetBytes)
    {
        m_device = device;
        m_memory = memory;
        m_numTiles = numTiles;
        m_tileW = tileW;
        m_tileH = tileH;
        m_wordsPerTile = DivCeil(tileW * tileH, 32u);
        m_offsetBytes = offsetBytes;

        EE_CORE_ASSERT(m_device != VK_NULL_HANDLE, "DeltaBitReader: device null");
        EE_CORE_ASSERT(m_memory != VK_NULL_HANDLE, "DeltaBitReader: memory null");
        EE_CORE_ASSERT(m_numTiles > 0, "DeltaBitReader: numTiles must be > 0");
        EE_CORE_ASSERT(m_tileW > 0 && m_tileH > 0, "DeltaBitReader: tile dims must be > 0");
        EE_CORE_ASSERT((m_offsetBytes % 4) == 0, "DeltaBitReader: offset must be 4-byte aligned");

        void* base = nullptr;
        VkResult res = vkMapMemory(m_device, m_memory, m_offsetBytes, VK_WHOLE_SIZE, 0, &base);
        if (res != VK_SUCCESS) return false;

        m_mappedRaw = base;
        m_base = static_cast<const uint32_t*>(base);
        return true;
    }

    void DeltaBitReader::Unmap()
    {
        if (m_mappedRaw) {
            vkUnmapMemory(m_device, m_memory);
            m_mappedRaw = nullptr;
            m_base = nullptr;
            m_numTiles = 0;
            m_tileW = 0;
            m_tileH = 0;
            m_wordsPerTile = 0;
            m_offsetBytes = 0;
            m_device = VK_NULL_HANDLE;
            m_memory = VK_NULL_HANDLE;
        }
    }

    const uint32_t* DeltaBitReader::Slice(uint32_t tileIdx) const
    {

        EE_CORE_ASSERT(tileIdx < m_numTiles, "DeltaBitReader::Slice tileIdx OOB (%u/%u)");
        return m_base + static_cast<size_t>(tileIdx) * m_wordsPerTile;
    }

    bool DeltaBitReader::Any(uint32_t tileIdx) const
    {
        const uint32_t* s = Slice(tileIdx);
        for (uint32_t i = 0; i < m_wordsPerTile; ++i)
            if (s[i] != 0u) return true;
        return false;
    }

    bool DeltaBitReader::FindFirstNonZeroTile(uint32_t& outTileIdx) const
    {
        EE_CORE_ASSERT(m_base != nullptr, "DeltaBitReader not mapped");
        for (uint32_t t = 0; t < m_numTiles; ++t) {
            const uint32_t* s = Slice(t);
            for (uint32_t i = 0; i < m_wordsPerTile; ++i) {
                if (s[i] != 0u) { outTileIdx = t; return true; }
            }
        }
        return false;
    }

    uint32_t DeltaBitReader::CountAlive(uint32_t tileIdx) const
    {
        const uint32_t* s = Slice(tileIdx);
        if (m_wordsPerTile == 0) return 0;

        const uint32_t usedBits = m_tileW * m_tileH;
        const uint32_t rem = usedBits % 32u;

        uint32_t sum = 0;

        // all full words except the last (if there is more than one)
        const uint32_t last = m_wordsPerTile - 1u;
        for (uint32_t i = 0; i < last; ++i)
            sum += Popcnt32(s[i]);

        // last word: mask off padding if needed
        const uint32_t lastWord = (rem == 0u)
            ? s[last]
            : (s[last] & ((1u << rem) - 1u));

        sum += Popcnt32(lastWord);
        return sum;
    }


    const uint32_t* DeltaBitReader::GetTileWords(uint32_t tileIdx) const
    {
        return Slice(tileIdx);
    }

    void DeltaBitReader::ForEachSetPixel(uint32_t tileIdx,
        const std::function<void(uint32_t, uint32_t)>& f) const
    {
        const uint32_t* s = Slice(tileIdx);

        // Scan each 32-bit word for set bits
        for (uint32_t wi = 0; wi < m_wordsPerTile; ++wi) {
            uint32_t word = s[wi];
            while (word) {
                // extract least-significant set bit
                const uint32_t lsb = word & -word;
                const uint32_t b = ctz32(word);
                word ^= lsb;

                const uint32_t idx = (wi << 5) + b;   // linear pixel index
                if (idx >= (uint32_t)(m_tileW * m_tileH)) continue; // guard padding

                const uint32_t y = idx / m_tileW;
                const uint32_t x = idx % m_tileW;
                f(x, y);
            }
        }
    }

    bool DeltaBitReader::AliveBBox(uint32_t tileIdx, int& minx, int& miny, int& maxx, int& maxy) const
    {
        const uint32_t* s = Slice(tileIdx);

        // Quick prune
        bool any = false;
        for (uint32_t i = 0; i < m_wordsPerTile; ++i) { if (s[i]) { any = true; break; } }
        if (!any) { return false; }

        minx = (int)m_tileW;
        miny = (int)m_tileH;
        maxx = -1;
        maxy = -1;

        ForEachSetPixel(tileIdx, [&](uint32_t x, uint32_t y) {
            if ((int)x < minx) minx = (int)x;
            if ((int)y < miny) miny = (int)y;
            if ((int)x > maxx) maxx = (int)x;
            if ((int)y > maxy) maxy = (int)y;
            });

        return (maxx >= minx) && (maxy >= miny);
    }

    void DeltaBitReader::CopyTileWords(uint32_t tileIdx, std::vector<uint32_t>& outWords) const
    {
        outWords.resize(m_wordsPerTile);
        std::memcpy(outWords.data(), Slice(tileIdx), m_wordsPerTile * sizeof(uint32_t));
    }

} 
