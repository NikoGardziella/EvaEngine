#pragma once
#include "glm/glm.hpp"

namespace Engine {

    class SkeletonRegistry {

    private:

        struct SkeletonAsset 
        {
            uint32_t id;
            std::vector<int16_t> parent;         // parent index per bone (-1 for root)
            std::vector<glm::mat4> invBind;      // inverse bind matrices
            std::vector<glm::mat4> restLocal;    // optional
        };


    public:
        uint32_t LoadGLTFSkeleton(const char* path);
        const SkeletonAsset& Get(uint32_t id) const;
    };
}


