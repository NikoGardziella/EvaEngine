#pragma once
#include <Engine/Scene/Components/NPC/Destruction/EnemyDestructibleComponent.h>
#include <Engine/Animation/3D/SkeletonRegistry.h>

namespace Engine {



	class SceneUtils
	{
	public:
		static EnemyPieceType ClassifyPieceTypeFromSubmeshName(const std::string& smName);

		static uint32_t BoneForPieceType(const SkeletonAsset& skel, EnemyPieceType t);
		
	private:
		static bool StrContains(const std::string& s, const char* needle);
	};

}

