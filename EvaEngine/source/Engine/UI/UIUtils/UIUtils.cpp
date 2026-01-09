#include "pch.h"
#include "UIUtils.h"


namespace Engine {

    glm::vec2 UIUtils::Anchor01(UIAnchorPreset a)
    {
        switch (a)
        {
        case UIAnchorPreset::TopLeft:      return { 0.0f, 0.0f };
        case UIAnchorPreset::TopCenter:    return { 0.5f, 0.0f };
        case UIAnchorPreset::TopRight:     return { 1.0f, 0.0f };
        case UIAnchorPreset::MiddleLeft:   return { 0.0f, 0.5f };
        case UIAnchorPreset::MiddleCenter: return { 0.5f, 0.5f };
        case UIAnchorPreset::MiddleRight:  return { 1.0f, 0.5f };
        case UIAnchorPreset::BottomLeft:   return { 0.0f, 1.0f };
        case UIAnchorPreset::BottomCenter: return { 0.5f, 1.0f };
        case UIAnchorPreset::BottomRight:  return { 1.0f, 1.0f };
        }
        return { 0,0 };
    }


    glm::vec2 UIUtils::ComputeTopLeftPx(const UITransform2D& t, glm::vec2 screenPx)
    {
        glm::vec2 a01 = Anchor01(t.anchor);
        glm::vec2 anchorPx = a01 * screenPx;
        glm::vec2 pivotPx = t.pivot * t.sizePx;
        return anchorPx + t.posPx - pivotPx;
    }
}