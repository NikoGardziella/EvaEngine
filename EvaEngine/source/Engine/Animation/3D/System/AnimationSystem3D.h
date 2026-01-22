#pragma once
#include <Engine/Animation/3D/SkeletonRegistry.h>
#include <Engine/Animation/3D/AnimationRegistry.h>
#include <Engine/Animation/3D/BonePaletteBuffer.h>

#include <glm/gtc/quaternion.hpp>

namespace Engine {

	class Scene;
	class AnimationSystem3D
	{
		struct AnimScratch3D
		{
			std::vector<glm::vec3> T;
			std::vector<glm::quat> R;
			std::vector<glm::vec3> S;

			std::vector<glm::vec3> TA;
			std::vector<glm::quat> RA;
			std::vector<glm::vec3> SA;

			std::vector<glm::vec3> TB;
			std::vector<glm::quat> RB;
			std::vector<glm::vec3> SB;

			std::vector<glm::vec3> TO;
			std::vector<glm::quat> RO;
			std::vector<glm::vec3> SO;

			std::vector<glm::mat4> model;
			std::vector<glm::mat4> finalMats;
		};


	public:
		void Update(Scene* scene, float dt, const SkeletonRegistry&, const AnimationRegistry&);


	private:
		void ApplyClipFullPose(uint32_t clipId, float t, uint32_t boneCount, const AnimationRegistry& animReg, std::vector<glm::vec3>& T, std::vector<glm::quat>& R, std::vector<glm::vec3>& S);
		//float AdvanceTime(uint32_t clipId, float& t, float dt, float playbackSpeed, const AnimationRegistry& animReg);
		float AdvanceTime(uint32_t clipId, float& t, float dt, float playbackSpeed, const AnimationRegistry& animReg, bool loop);
		void ApplyClip(uint32_t clipId, float t, float weight, uint32_t boneCount, const AnimationRegistry& animReg, std::vector<glm::vec3>& locT, std::vector<glm::quat>& locR, std::vector<glm::vec3>& locS);
	
		inline int FindKey(const std::vector<float>& times, float t);

		inline void SampleChannel(const AnimChannel& ch, float t, glm::vec3& T, glm::quat& R, glm::vec3& S);
	};


}

