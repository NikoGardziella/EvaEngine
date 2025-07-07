#pragma once
#include <Engine/Scene/Entity.h>

namespace Engine {

	class EditorUtils
	{
	public:
		static Entity FindTileAtPosition(Ref<Scene> scene, const glm::vec2& worldPosition);
	};


}

