#pragma once
#include <cstdint>
#include "glm/glm.hpp"

namespace Engine {



    struct GPUDirectionalLight
    {
        glm::vec4 direction_intensity; // xyz dir, w intensity
        glm::vec4 color;               // rgb, a unused
    };

    struct GPUPointLight
    {
        glm::vec4 position_radius;     // xyz pos, w radius
        glm::vec4 color_intensity;     // rgb color, w intensity
    };

    struct GPUSpotLight
    {
        glm::vec4 position_range;      // xyz pos, w range
        glm::vec4 direction_inner;     // xyz dir, w cos(inner)
        glm::vec4 color_outer;         // rgb color, w cos(outer)
        glm::vec4 intensity_pad;       // x intensity
    };

    struct GPULightHeader
    {
        uint32_t numDir;
        uint32_t numPoint;
        uint32_t numSpot;
        uint32_t pad;
    };

}