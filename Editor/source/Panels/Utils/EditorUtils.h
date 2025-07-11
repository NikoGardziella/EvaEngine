#pragma once
#include <Engine/Scene/Entity.h>

namespace Engine {

	class EditorUtils
	{
	public:
		static Entity FindEntityAtPosition(Ref<Scene> scene, const glm::vec2& worldPosition);
		//static Entity FindTileAtPosition(Ref<Scene> scene, const glm::vec2& worldPosition);
		static void EditorUtils::DeleteTileAtPosition(Entity entity, const glm::vec2& worldPosition);

	};


}

