#pragma once
#include <cstdint>
#include <Engine/Animation/3D/System/AnimUtils/AnimUtils.h>
#include <Engine/Scene/Components/Animation/NpcAnimationControllerComponent.h>



namespace Engine {

	class SpawnUtils
	{
	public:
		static void ResolveZombieClips(NpcAnimationControllerComponent& animControllerComp, const Engine::MeshAsset& mesh);

	private:
		static uint32_t FindClipId(const char* name);
		static uint32_t FindClipIdChecked(const char* name, uint32_t expectedSkeletonId, uint32_t expectedBoneCount);
	};
}


