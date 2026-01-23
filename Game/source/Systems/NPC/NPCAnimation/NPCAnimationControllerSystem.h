#pragma once
#include <Engine/Scene/Scene.h>
#include <Engine/Scene/Components/Render/3D/AnimatorComponent.h>
#include <Engine/Scene/Components/Animation/NpcAnimationControllerComponent.h>
#include <Engine/Scene/Components/NPC/NpcBodyStateComponent.h>
#include <Engine/Animation/3D/AnimationRegistry.h>


class Scene;
class NPCAnimationControllerSystem
{

public:
	static void UpdateNPCAnimationControllerSystem(float deltaTime, Engine::Scene* scene);
	
private:

	static void UpdateOneShotB(Engine::Animator3DComponent& anim, NpcAnimationControllerComponent& ctrl, float dt);
	static uint32_t TransitionClip(const NpcBodyStateComponent& body, const NpcAnimationControllerComponent& ctrl);
	static float GetClipDurationOrZero(const Engine::AnimationRegistry& animReg, uint32_t clip);

	static float Smooth01(float x);

	static void UpdateBaseCrossfade(Engine::Animator3DComponent& anim, NpcAnimationControllerComponent& ctrl, float dt);

	//static void UpdateOneShot(Engine::Animator3DComponent& anim, NpcAnimationControllerComponent& ctrl, float dt);

};

