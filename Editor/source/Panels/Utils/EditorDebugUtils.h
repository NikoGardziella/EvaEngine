#pragma once

#include <Engine/Scene/Entity.h>
#include <Engine/Core/Core.h>


namespace Engine {

	class Scene;
	class EditorDebugUtils
	{
	public:
		static void PrintAllEntities(Scene* scene);
		static void EditorDebugUtils::DrawAreaDebugBounds(Ref<Scene> scene);

		static void DrawWorldAxes(const glm::vec2& cameraWorldPos, float extent);

		static void DrawIsometricGrid(const glm::vec2& cameraWorldPos, float extent);


	};

}


