#include "pch.h"
#include "Math.h"

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/matrix_decompose.hpp>
#include <glm/gtx/quaternion.hpp>

namespace Engine {

	namespace Math {




        bool DecomposeTransform(const glm::mat4& transform, glm::vec3& outTranslation, glm::quat& outRotation, glm::vec3& outScale)
        {
            using namespace glm;

            // Extract translation
            outTranslation = vec3(transform[3]);

            // Extract scale (length of each column vector)
            vec3 scale;
            scale.x = length(vec3(transform[0]));
            scale.y = length(vec3(transform[1]));
            scale.z = length(vec3(transform[2]));

            // Normalize the matrix to remove scale
            mat3 rotationMatrix = mat3(transform);
            rotationMatrix[0] /= scale.x;
            rotationMatrix[1] /= scale.y;
            rotationMatrix[2] /= scale.z;

            // Convert rotation matrix to quaternion
            outRotation = quat_cast(rotationMatrix);

            // Output scale
            outScale = scale;

            return true; // Successful decomposition
        }





	}
}
