#include "pch.h"
#include "AssetManagerUtils.h"


namespace Engine {

    void AssetManagerUtils::ComputePivotFromAlpha(const uint8_t* rgba, int w, int h, int alphaThresh,
        int& outPivotYOffsetPx, int& outPivotXCenterOffsetPx)
    {
        outPivotYOffsetPx = 0;
        outPivotXCenterOffsetPx = 0;

        // Find first opaque row from the bottom (image data is top-down: bottom is row h-1)
        int firstSolidY = -1;
        for (int y = h - 1; y >= 0; --y) {
            const uint8_t* row = rgba + (size_t(y) * w) * 4;
            for (int x = 0; x < w; ++x) {
                if (row[x * 4 + 3] > alphaThresh) { firstSolidY = y; break; }
            }
            if (firstSolidY >= 0) break;
        }
        if (firstSolidY < 0) {
            // fully transparent (unlikely)—leave zeros
            return;
        }
        // Transparent rows below the first solid row:
        outPivotYOffsetPx = (h - 1) - firstSolidY;

        // Estimate horizontal foot center from a small band above the first solid row
        const int band = std::min(8, h);
        int minx = w, maxx = -1;
        for (int y = firstSolidY; y >= std::max(0, firstSolidY - band + 1); --y) {
            const uint8_t* row = rgba + (size_t(y) * w) * 4;
            for (int x = 0; x < w; ++x) {
                if (row[x * 4 + 3] > alphaThresh) {
                    if (x < minx) minx = x;
                    if (x > maxx) maxx = x;
                }
            }
        }
        if (maxx >= minx) {
            const int center = (minx + maxx) / 2;
            outPivotXCenterOffsetPx = center - (w / 2); // +right, -left
        }
    }


}
