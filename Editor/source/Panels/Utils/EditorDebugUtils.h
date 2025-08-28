#pragma once

#include <Engine/Scene/Entity.h>


namespace Engine {

	class Scene;
	class EditorDebugUtils
	{
	public:
		static void PrintAllEntities(Scene* scene);
	};

}


