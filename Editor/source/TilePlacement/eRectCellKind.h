#pragma once
#include <cstdint>

namespace Engine {


    enum class eRectCellKind : uint8_t
    {
        None = 0,
        TopEdge,
        BottomEdge,
        LeftEdge,
        RightEdge,
        TopLeftCorner,
        TopRightCorner,
        BottomLeftCorner,
        BottomRightCorner,
        Interior
    };

}