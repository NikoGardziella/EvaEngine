#pragma once
#include "glm/glm.hpp"
#include <algorithm>
#include <cmath>
#include "AnimationEnums.h"
#include <glm/ext/scalar_constants.hpp>
#include <glm/gtx/quaternion.hpp>

namespace Engine {

    // Map angle (radians, 0 is +X) to Dir8 with mild hysteresis to prevent flicker
    struct DirectionSelector {
        float lastAngle = 0.0f;
        float hysteresisDeg = 10.0f;

        static inline float Normalize(float a)
        {
            while (a <= -glm::pi<float>())
            {
                a += 2 * glm::pi<float>();
            }

            while (a > glm::pi<float>())
            {
                a -= 2 * glm::pi<float>();
            }
            return a;
        }

        Dir8 FromAngle(float radians)
        {
            radians = Normalize(radians);

            // lock small changes
            const float diff = std::abs(Normalize(radians - lastAngle));
            if (diff < glm::radians(hysteresisDeg))
                radians = lastAngle;
            else
                lastAngle = radians;

            // 45 deg sectors starting at E, CCW
            const float sector = glm::pi<float>() / 4.0f;
            int idx = int(std::round(radians / sector));
            idx = (idx + 8) & 7; // [0..7], 0 is E, then NE,N,NW,W,SW,S,SE

            // sheet order is E, SE, S, SW, W, NW, N, NE (clockwise)
            static constexpr Dir8 ccwToClockwiseSheet[8] =
            { Dir8::E, Dir8::NE, Dir8::N, Dir8::NW, Dir8::W, Dir8::SW, Dir8::S, Dir8::SE };

            return ccwToClockwiseSheet[idx];
        }

        Dir8 FromVector(const glm::vec2& v)
        {
            if (glm::length2(v) < 1e-6f)
            {
                return Dir8::S;
            }
            return FromAngle(std::atan2(v.y, v.x));
        }
    };

}
