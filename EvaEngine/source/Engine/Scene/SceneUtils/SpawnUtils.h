#pragma once
#include <cstdint>
#include <Engine/Animation/3D/System/AnimUtils/AnimUtils.h>
#include <Engine/Scene/Components/Animation/NpcAnimationControllerComponent.h>
#include <Engine/Scene/Entity.h>
#include "Prefabs/NPCprefab.h"
#include <Engine/Animation/3D/MeshRegistry.h>



namespace Engine {

	class SpawnUtils
	{
	public:
		static void ResolveZombieClips(NpcAnimationControllerComponent& animControllerComp, const Engine::MeshAsset& mesh);

		static glm::vec2 RandomUnit2D(uint32_t& seed);

		static float RandomRange(uint32_t& seed, float a, float b);

		static Entity SpawnNPCFromPrefab(Scene* scene, const NpcPrefab& prefab, const glm::vec3& worldPos);
		static float RandomFloat(float minV, float maxV);

	private:
		static uint32_t FindClipId(const char* name);
		static uint32_t FindClipIdChecked(const char* name, uint32_t expectedSkeletonId, uint32_t expectedBoneCount);
	};
}


