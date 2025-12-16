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
		static void GLTF_GatherNodesDFS(const tinygltf::Model& model, int nodeIndex, const glm::mat4& parentWorld, std::vector<int>& outNodeIndices, std::vector<glm::mat4>& outNodeWorlds);
		static float GuessImportScaleFromBounds(const glm::vec3& minL, const glm::vec3& maxL);
	
	private:
		static glm::mat4 GLTF_NodeLocalMatrix(const tinygltf::Node& n);
	};

}

