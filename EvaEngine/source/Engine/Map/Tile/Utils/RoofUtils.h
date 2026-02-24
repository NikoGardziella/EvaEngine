#pragma once

#include "glm/glm.hpp"
#include <Engine/Map/Tile/RoofSystem.h>
namespace Engine {


    class RoofUtils {


    public:

        static inline bool AABBOverlaps(const glm::ivec2& aMin, const glm::ivec2& aMax,
            const glm::ivec2& bMin, const glm::ivec2& bMax)
        {
            if (aMax.x < bMin.x || bMax.x < aMin.x) return false;
            if (aMax.y < bMin.y || bMax.y < aMin.y) return false;
            return true;
        }

        static inline int Quantize(float v, float step)
        {
            // round to nearest step index
            return (int)std::floor(v / step + 0.5f);
        }

        static inline float Dequantize(int i, float step)
        {
            return (float)i * step;
        }

        static inline glm::ivec2 AABBMin(const glm::ivec2& a, const glm::ivec2& b)
        {
            return { std::min(a.x, b.x), std::min(a.y, b.y) };
        }
        static inline glm::ivec2 AABBMax(const glm::ivec2& a, const glm::ivec2& b)
        {
            return { std::max(a.x, b.x), std::max(a.y, b.y) };
        }
        static void MarkDirtyRadius(RoofSystemState& st, const glm::ivec2& p, int r)
        {
            DirtyAABB box;
            box.min = { p.x - r, p.y - r };
            box.max = { p.x + r, p.y + r };
            st.dirty.push_back(box);
        }

        static void MergeDirtyAABBs(RoofSystemState& st)
        {
            EE_PROFILE_FUNCTION();

            st.dirtyMerged.clear();
            if (st.dirty.empty()) return;

            st.dirtyMerged = st.dirty;
            st.dirty.clear();

            // Simple O(n^2) merge. Dirty list should stay small.
            bool mergedAny = true;
            while (mergedAny)
            {
                mergedAny = false;
                for (size_t i = 0; i < st.dirtyMerged.size(); ++i)
                {
                    for (size_t j = i + 1; j < st.dirtyMerged.size(); )
                    {
                        auto& a = st.dirtyMerged[i];
                        auto& b = st.dirtyMerged[j];

                        if (AABBOverlaps(a.min, a.max, b.min, b.max))
                        {
                            a.min = AABBMin(a.min, b.min);
                            a.max = AABBMax(a.max, b.max);
                            st.dirtyMerged.erase(st.dirtyMerged.begin() + j);
                            mergedAny = true;
                        }
                        else
                        {
                            ++j;
                        }
                    }
                }
            }
        }

        static inline size_t LocalIndex(int x, int y, int w)
        {
            return (size_t)y * (size_t)w + (size_t)x;
        }

    };

}
