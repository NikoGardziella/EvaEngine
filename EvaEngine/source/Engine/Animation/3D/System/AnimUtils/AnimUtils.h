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

		static float FindClipDuration(uint32_t clipId);

		static void StartOneShot(Engine::Animator3DComponent& anim, NpcAnimationControllerComponent& ctrl, const Engine::AnimationRegistry& animReg, uint32_t clipBId, AIState returnState);


		static void SetLoopClip(Engine::Animator3DComponent& anim, NpcAnimationControllerComponent& ctrl, uint32_t clipId);
	};

}

