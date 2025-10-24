#include "pch.h"
#include "DeltaBitReader.h"



namespace Engine {

#include "DeltaBitReader.h"

    bool DeltaBitReader::Map(VkDevice device, VkDeviceMemory memory, uint32_t numTiles)
    {
        m_device = device;
        m_memory = memory;
        m_numTiles = numTiles;

        void* base = nullptr;
        if (vkMapMemory(m_device, m_memory, 0, VK_WHOLE_SIZE, 0, &base) != VK_SUCCESS)
            return false;

        m_mapped = base;
        m_base = static_cast<const uint32_t*>(base);
        return true;
    }

    void DeltaBitReader::Unmap()
    {
        if (m_mapped) {
            vkUnmapMemory(m_device, m_memory);
            m_mapped = nullptr;
            m_base = nullptr;
            m_numTiles = 0;
        }
    }

    bool DeltaBitReader::Any(uint32_t tileIdx) const
    {
        const uint32_t* s = Slice(tileIdx);
        for (uint32_t i = 0; i < WORDS_PER_TILE; ++i)
            if (s[i] != 0u) return true;
        return false;
    }

    void DeltaBitReader::MergeIntoAlive(uint32_t tileIdx, DirtyTileRuntime& t) const
    {
        EnsureAliveAllocated(t);
        ForEachSetPixel(tileIdx, [&](uint32_t px, uint32_t py) {
            const uint32_t idx = py * TILE_W + px;
            t.aliveBits[idx >> 3] &= uint8_t(~(1u << (idx & 7)));
            // If you keep full health too:
            // if (!t.healthCPU.empty()) t.healthCPU[idx] = 0;
            });
    }

    bool DeltaBitReader::DirtyCellsBBox(uint32_t tileIdx,
        uint32_t DIRTY_CELLS_PER_TILE,
        uint32_t& outMinX, uint32_t& outMinY,
        uint32_t& outMaxX, uint32_t& outMaxY) const
    {
        const uint32_t CELLS = DIRTY_CELLS_PER_TILE;
        const uint32_t cellPxW = (TILE_W + CELLS - 1) / CELLS; // ceil
        const uint32_t cellPxH = (TILE_H + CELLS - 1) / CELLS;

        bool any = false;
        uint32_t cminX = CELLS, cminY = CELLS;
        uint32_t cmaxX = 0, cmaxY = 0;

        ForEachSetPixel(tileIdx, [&](uint32_t px, uint32_t py) {
            uint32_t cx = px / cellPxW;
            uint32_t cy = py / cellPxH;
            any = true;
            if (cx < cminX) cminX = cx;
            if (cy < cminY) cminY = cy;
            if (cx > cmaxX) cmaxX = cx;
            if (cy > cmaxY) cmaxY = cy;
            });

        if (!any) return false;

        outMinX = cminX; outMinY = cminY;
        outMaxX = cmaxX + 1; outMaxY = cmaxY + 1; // half-open
        return true;
    }


}