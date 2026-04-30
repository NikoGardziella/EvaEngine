#pragma once
#include <cstdint>
#include "glm/glm.hpp"

struct StairsComponent
{
    int16_t FromFloor = 0;
    int16_t ToFloor = 1;

    glm::vec2 EntryDir = glm::vec2(0.0f, 1.0f);
    float TriggerRadius = 1.0f;
};