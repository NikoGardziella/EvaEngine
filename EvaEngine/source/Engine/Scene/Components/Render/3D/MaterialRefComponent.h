#pragma once
#include <cstdint>
#include "glm/glm.hpp"

namespace Engine {

    struct MaterialRefComponent
    {
        // bindless texture indices
        uint32_t baseColorTex = 0xFFFFFFFFu;
        uint32_t normalTex = 0xFFFFFFFFu;
        uint32_t ormTex = 0xFFFFFFFFu; // occlusion-roughness-metallic packed
        uint32_t emissiveTex = 0xFFFFFFFFu;
        glm::vec4 baseColorFactor{ 1 };
        float metallicFactor = 1.0f;
        float roughnessFactor = 1.0f;
        uint32_t flags = 0; // alpha mode, double-sided, etc.
        uint32_t  materialId;
    };
}