#pragma once
#include <cstdint>

struct FloorComponent
{
    int16_t Floor = 0;
    int16_t TargetFloor = 0;
    float VisualFloor = 0.0f;
    float FloorT = 0.0f;      // 0 -> 1 while climbing
    bool IsChangingFloor = false;
    float ClimbSpeed = 1.5f;


    bool IsOnStairs = false;
    glm::vec2 ActiveStairBottom;
    glm::vec2 ActiveStairTop;
    float ActiveStairHalfWidth = 0.0f;
};