#pragma once

#include <Engine/Scene/Entity.h>


namespace Engine {

	class EditorDebugUtils
	{
	public:
		static void PrintAllEntities(entt::registry& registry);
	};

}


