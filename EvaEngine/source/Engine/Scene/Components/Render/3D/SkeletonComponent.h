#pragma once
#include <cstdint>

namespace Engine {

    struct SkeletonComponent
    {
        uint32_t skeletonId;         // registry index
        uint32_t boneCount;
        uint32_t boneOffset;         // offset into a global bone palette buffer (mat4[])
        uint32_t boneBase;
    };
}
