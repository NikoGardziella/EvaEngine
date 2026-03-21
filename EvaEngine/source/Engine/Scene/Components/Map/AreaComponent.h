#pragma once
#include <cstdint>
#include "glm/glm.hpp"

enum class AreaType : uint32_t
{
    Generic = 0,
    Building,   // Hides roofs
    Forest,     // Fades tree tops
    MusicZone,  // Only changes audio
    Indoor      // Changes lighting/ambient
};

struct AreaComponent {
    uint32_t Id;
    AreaType Type;
    glm::vec2 Min;
    glm::vec2 Max;

    // Metadata for game logic
    uint32_t musicID = 0;
    bool hideRoofs = false;


    void Expand(glm::vec2 point) 
    {
        Min.x = std::min(Min.x, point.x);
        Min.y = std::min(Min.y, point.y);
        Max.x = std::max(Max.x, point.x);
        Max.y = std::max(Max.y, point.y);
    }
};