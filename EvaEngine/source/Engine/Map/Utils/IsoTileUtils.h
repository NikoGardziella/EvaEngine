
#include <glm/glm.hpp>
#include <Engine/Core/Config.h>

namespace Engine {

    class IsoTileUtils {
       
    public:
        static inline glm::ivec2 WorldToIsoCellInt(const glm::vec2& p)
        {
            float tX = p.x / (GRID_TILE_W * 0.5f);
            float tY = p.y / (GRID_TILE_H * 0.5f);
            float u = 0.5f * (tY + tX);
            float v = 0.5f * (tY - tX);
            return glm::ivec2(glm::round(glm::vec2(u, v)));
        }

        static inline glm::ivec2 WorldToIsoCell(const glm::vec2& p)
        {
            float tX = p.x / (GRID_TILE_W * 0.5f);
            float tY = p.y / (GRID_TILE_H * 0.5f);
            float u = 0.5f * (tY + tX);
            float v = 0.5f * (tY - tX);
            return glm::vec2(u, v);
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

       

        static inline glm::vec2 RotateCW(const glm::vec2& v, float angleRad)
        {
            float c = cos(angleRad);
            float s = sin(angleRad);

            return glm::vec2(
                v.x * c + v.y * s,
                -v.x * s + v.y * c
            );
        }

        static inline glm::vec2 StairDirectionToIsoVector(eTileDirection dir)
        {
            // this is diagonial values
            glm::vec2 base;

            // this can be ttweaked inside the switch statement because the angles are not exactly diagonial
            float angle = 0.0f;

            switch (dir)
            {
            case eTileDirection::North:
                base = glm::vec2(-1.0f, 1.0f);
                angle = glm::radians(10.0f);
                break;

            case eTileDirection::East:
                base = glm::vec2(1.0f, 1.0f);
                angle = glm::radians(10.0f);
                break;

            case eTileDirection::South:
                base = glm::vec2(1.0f, -1.0f);
                angle = glm::radians(10.0f);
                break;

            case eTileDirection::West:
                base = glm::vec2(-1.0f, -1.0f);
                angle = glm::radians(10.0f);
                break;

            default:
                base = glm::vec2(-1.0f, 1.0f);
                angle = glm::radians(10.0f);
                break;
            }

            base = glm::normalize(base);
            return glm::normalize(RotateCW(base, angle));
        }

    };

}