#pragma once

#include <glm/gtx/transform.hpp>
#include <glm/gtc/quaternion.hpp>


class AnimationUtils {

public:
    static inline glm::mat4 TRS(const glm::vec3& t, const glm::quat& r, const glm::vec3& s)
    {
        // T * R * S
        glm::mat4 T = glm::translate(glm::mat4(1.0f), t);
        glm::mat4 R = glm::mat4_cast(r);
        glm::mat4 S = glm::scale(glm::mat4(1.0f), s);
        return T * R * S;
    }

};

