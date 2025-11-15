#pragma once
#include <cstdint>

namespace Engine {

    struct SkinnedMeshRefComponent
    {
        uint32_t meshId;             // uses vertex weights/indices streams
        uint32_t submeshCount;
        uint32_t submeshFirst;
    };

}


