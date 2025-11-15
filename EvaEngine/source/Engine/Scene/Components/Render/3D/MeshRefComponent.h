#pragma once
#include <cstdint>

namespace Engine {


    struct MeshRefComponent
    {
        uint32_t meshId;       // index to MeshRegistry
        uint32_t submeshFirst; // submesh range
        uint32_t submeshCount;
    };
}