#include <Engine/Core/Core.h>

#include <glm/glm.hpp>

namespace Engine {

    class IsoTileUtils {
       
    public:
        static inline glm::ivec2 WorldToIsoCell(const glm::vec2& p)
        {
            float tX = p.x / (GRID_TILE_W * 0.5f);
            float tY = p.y / (GRID_TILE_H * 0.5f);
            float u = 0.5f * (tY + tX);
            float v = 0.5f * (tY - tX);
            return glm::ivec2(glm::round(glm::vec2(u, v)));
        }


        static inline glm::vec2 IsoToWorldGround(const glm::ivec2& c)
        {
            return {
                (c.x - c.y) * (GRID_TILE_W * 0.5f),
                (c.x + c.y) * (GRID_TILE_H * 0.5f)
            };
        }

        static inline glm::vec2 IsoDeltaToWorldDeltaGround(const glm::ivec2& d)
        {
            return
            {
                (d.x - d.y) * (GRID_TILE_W * 0.5f),
                (d.x + d.y) * (GRID_TILE_H * 0.5f)
            };
        }       
    };

}