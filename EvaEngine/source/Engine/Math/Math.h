#pragma once

#include <glm/glm.hpp>

namespace Engine {

	namespace Math {



		bool DecomposeTransform(const glm::mat4& transform, glm::vec3& outTranslation, glm::quat& outRotation, glm::vec3& outScale);

	}


}
