#include "pch.h"
#include "TileManagerUtils.h"

namespace Engine {

    void TileManagerUtils::BuildPropsForInstance(
        const TileInfo& ti,                     // per-instance
        const TileProperties& props,            // pivot/foot/category defaults per tile type
        const TileSource& src,                  // base RGBA for this tile type
        std::vector<uint8_t>& outPropsRGBA8UI)  // size = w*h*4
    {
        const int w = src.width, h = src.height;
        outPropsRGBA8UI.assign(size_t(w) * size_t(h) * 4, 0);

        const uint8_t baseHealth = ti.TileHealth ? uint8_t(std::min<uint32_t>(ti.TileHealth, 255u)) : 255u;

        constexpr uint8_t FLAG_ANCHOR = 1u << 1;
        constexpr uint8_t FLAG_SOLID = 1u << 2;

        // category nibble in high 4 bits
        const uint8_t catNibble = uint8_t((uint32_t(ti.Category) & 0xF) << 4);

        // Pivot/foot band in rows from bottom
        const int pivotY = std::clamp<int>(props.pivotYOffsetPx, 0, std::max(0, h - 1));
        const int footRows = std::clamp<int>(props.collisionFootRowsPx, 0, pivotY);
        const int bandStartY = std::max(0, pivotY - footRows);
        const int bandEndY = pivotY; // [start, end)

        const uint8_t* s = src.rgba.data();
        for (int y = 0; y < h; ++y)
        {
            const size_t row = size_t(y) * size_t(w) * 4;
            const bool inFootBand = (y >= bandStartY && y < bandEndY);

            for (int x = 0; x < w; ++x)
            {
                const size_t i = row + size_t(x) * 4;
                const uint8_t a = s[i + 3];

                // R = health
                uint8_t R = (a != 0 ? baseHealth : 0);
                if (inFootBand && R == 0) R = baseHealth; // optional: foot band collides even if alpha==0

                // G = rows-above-pivot (0 at/under pivot or invisible)
                uint8_t G = 0;
                if (a != 0) {
                    const int rowAbove = (h - 1 - y) - pivotY;
                    if (rowAbove > 0) G = uint8_t(std::min(rowAbove, 255));
                }

                // B = optional per-instance id / timer init (0 for now)
                uint8_t B = 0;

                // A = flags|category
                uint8_t A = 0;
                if (a != 0 || inFootBand) A |= FLAG_SOLID;
                if (inFootBand)           A |= FLAG_ANCHOR;
                A |= catNibble;

                outPropsRGBA8UI[i + 0] = R;
                outPropsRGBA8UI[i + 1] = G;
                outPropsRGBA8UI[i + 2] = B;
                outPropsRGBA8UI[i + 3] = A;
            }
        }
    }

}