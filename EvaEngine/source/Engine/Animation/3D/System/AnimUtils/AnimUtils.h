#pragma once
#include <Engine/Scene/Components/Animation/NpcAnimationControllerComponent.h>
#include <Engine/Scene/Components/Render/3D/AnimatorComponent.h>
#include <Engine/Scene/Components/NPC/NpcAIStateComponent.h>


// remove from engine namespace?
namespace Engine {

	
	class Entity;
	class AnimUtils
	{
	public:

		static void StartBaseCrossfade(Engine::Animator3DComponent& anim, NpcAnimationControllerComponent& ctrl, uint32_t nextClip, float durationSeconds);

		static float FindClipDuration(uint32_t clipId);

	
		static void StartOneShotClipA(Engine::Animator3DComponent& anim, NpcAnimationControllerComponent& ctrl, Engine::AnimationRegistry& animReg, uint32_t clipAId, AIState returnState);
		static void StartOneShotClipB(Engine::Animator3DComponent& anim,
			NpcAnimationControllerComponent& ctrl,
			const Engine::AnimationRegistry& animReg,
			uint32_t clipBId,
			AIState returnState,
			float maxBlend /*= 0.6f*/);

		static void SetLoopClip(Engine::Animator3DComponent& anim, NpcAnimationControllerComponent& ctrl, uint32_t clipId);
	};

}

