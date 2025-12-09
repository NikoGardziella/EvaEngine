#pragma once
#include "glm/glm.hpp"
#include <Engine/Scene/Component.h>


namespace Engine {
	
	class Render3DUtils
    {

    public:
        inline static glm::mat4 Build3DWorld(const TransformComponent& t)
        {

            float footYOffset = 10.0f;
            // 2D world position
            glm::mat4 T = glm::translate(glm::mat4(1.0f),
                glm::vec3(t.Translation.x, t.Translation.y, 0.0f));

            // 2D facing angle
            glm::mat4 Rz = glm::rotate(glm::mat4(1.0f),
                t.Rotation.z,
                glm::vec3(0, 0, 1));

            // Move mesh down in its local space so the feet hit (0,0,0)
            // sign and axis depend on your asset; start with Y, tweak from there
            glm::mat4 Off = glm::translate(glm::mat4(1.0f),
                glm::vec3(0.0f, -footYOffset, 0.0f));

            return T * Rz * Off;
        }

	};



}