#pragma once
#include <cstdint>
#include <Engine/Animation/3D/System/AnimUtils/AnimUtils.h>
#include <Engine/Scene/Components/Animation/NpcAnimationControllerComponent.h>



namespace Engine {

	class SpawnUtils
	{
	public:
		static void ResolveZombieClips(NpcAnimationControllerComponent& animControllerComp);

	private:
		static uint32_t FindClipId(const char* name);
	};
}


