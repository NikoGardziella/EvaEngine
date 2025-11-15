#pragma once
#include <vector>
#include <utility>
#include <glm/glm.hpp>

namespace Engine {

    struct VulkanCtx {
        void* paletteMappedPtr = nullptr; // set by your renderer per frame
    };

    class BonePaletteBuffer
    {
    public:
        uint32_t Allocate(uint32_t boneCount);
        void     Free(uint32_t boneBase, uint32_t boneCount);
        void     Upload(uint32_t boneBase, const glm::mat4* mats, uint32_t boneCount);
        void     FlushToGPU(VulkanCtx& vk);

    private:
        std::vector<glm::mat4> m_cpuMats;
        std::vector<std::pair<uint32_t, uint32_t>> m_dirtyRanges;
    };

}
