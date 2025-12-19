#pragma once
#include "Engine.h"
#include <Engine/Scene/Components/Render/3D/AnimatorComponent.h>
#include <Engine/Scene/Components/Animation/NpcAnimationControllerComponent.h>


class Scene;
class NPCAnimationControllerSystem
{

public:
	static void UpdateNPCAnimationControllerSystem(float deltaTime, Engine::Scene* scene);

	static void UpdateOneShot(Engine::Animator3DComponent& anim, NpcAnimationControllerComponent& ctrl, float dt);

};

