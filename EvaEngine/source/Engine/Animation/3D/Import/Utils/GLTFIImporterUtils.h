#pragma once
#include <tiny_gltf.h>
#include <Engine/Animation/3D/SkeletonRegistry.h>
#include <Engine/Animation/3D/AnimationRegistry.h>

namespace Engine
{

	class GLTFIImporterUtils
	{
	public:

		static uint32_t LoadSkeletonFromModel(const tinygltf::Model& model, SkeletonRegistry& skelReg, const char* debugName);

		static void LoadClipsFromModel(const tinygltf::Model& model, AnimationRegistry& animReg, uint32_t skeletonId, const char* debugName, std::vector<uint32_t>& outClipIds);

	};

}

