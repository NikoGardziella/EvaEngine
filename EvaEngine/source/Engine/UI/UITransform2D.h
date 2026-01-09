#pragma once
#include <cstdint>
#include "glm/glm.hpp"

enum class UIAnchorPreset
{
    TopLeft, TopCenter, TopRight,
    MiddleLeft, MiddleCenter, MiddleRight,
    BottomLeft, BottomCenter, BottomRight
};

struct UITransform2D
{
    UIAnchorPreset anchor = UIAnchorPreset::TopLeft;
    glm::vec2 posPx = { 0,0 };     // offset from anchor
    glm::vec2 sizePx = { 64,64 };
    glm::vec2 pivot = { 0,0 };     // 0..1, (0,0)=top-left, (0.5,0.5)=center
    int32_t layer = 0;
    bool visible = true;
};
