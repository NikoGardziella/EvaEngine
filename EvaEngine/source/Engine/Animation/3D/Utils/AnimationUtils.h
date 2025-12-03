#pragma once

#include <glm/gtx/transform.hpp>
#include <glm/gtc/quaternion.hpp>


class AnimationUtils {

public:

    static inline void DecomposeTRS(const glm::mat4& M, glm::vec3& outT,  glm::quat& outR, glm::vec3& outS)
    {
        // Translation is the 4th column
        outT = glm::vec3(M[3]);

        // Extract scale from column lengths
        glm::vec3 col0 = glm::vec3(M[0]);
        glm::vec3 col1 = glm::vec3(M[1]);
        glm::vec3 col2 = glm::vec3(M[2]);

        outS.x = glm::length(col0);
        outS.y = glm::length(col1);
        outS.z = glm::length(col2);

        // Avoid divide-by-zero
        glm::vec3 invS(
            outS.x != 0.0f ? 1.0f / outS.x : 0.0f,
            outS.y != 0.0f ? 1.0f / outS.y : 0.0f,
            outS.z != 0.0f ? 1.0f / outS.z : 0.0f
        );

        glm::mat3 Rm(
            col0 * invS.x,
            col1 * invS.y,
            col2 * invS.z
        );

        outR = glm::quat_cast(Rm);
    }


    static inline glm::mat4 TRS(const glm::vec3& t, const glm::quat& r, const glm::vec3& s)
    {
        // T * R * S
        glm::mat4 M = glm::mat4_cast(r);

        // Apply scale by scaling the basis vectors
        M[0] *= s.x;  // X basis
        M[1] *= s.y;  // Y basis
        M[2] *= s.z;  // Z basis
        // Set translation
        M[3] = glm::vec4(t, 1.0f);
        return M;
    }

    template<typename VecT>
    static inline void EnsureSize(VecT& v, uint32_t count)
    {
        if (v.size() < count)
            v.resize(count);
    }


};

