#include "pch.h"
#include "BonePaletteBuffer.h"
#include <algorithm>

namespace Engine {

    uint32_t BonePaletteBuffer::Allocate(uint32_t boneCount) 
    {
        // simple linear bump, no free list to start
        uint32_t base = (uint32_t)m_cpuMats.size();
        m_cpuMats.resize(base + boneCount, glm::mat4(1.0f));
        m_dirtyRanges.emplace_back(base, boneCount);
        return base;
    }

    void BonePaletteBuffer::Free(uint32_t, uint32_t)
    {
        // TODO: implement free list
    }

    void BonePaletteBuffer::Upload(uint32_t boneBase, const glm::mat4* mats, uint32_t boneCount)
    {
        if (boneBase + boneCount > m_cpuMats.size()) return;
        std::memcpy(&m_cpuMats[boneBase], mats, sizeof(glm::mat4) * boneCount);
        m_dirtyRanges.emplace_back(boneBase, boneCount);
    }

    void BonePaletteBuffer::FlushToGPU(VulkanCtx& vk) 
    {
        // Wire this to  existing renderer:
        // Map your VkBuffer (palette SSBO) once per frame and memcpy dirty spans.
        // For now we assume one contiguous buffer.
        if (!vk.paletteMappedPtr) return;

        for (auto [base, count] : m_dirtyRanges)
        {
            std::memcpy((char*)vk.paletteMappedPtr + base * sizeof(glm::mat4),
                &m_cpuMats[base], sizeof(glm::mat4) * count);
        }
        m_dirtyRanges.clear();
    }

}
