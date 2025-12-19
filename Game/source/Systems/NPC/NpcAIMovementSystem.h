#pragma once
#include <Engine/Scene/Scene.h>
#include "Engine.h"
#include <Engine/Scene/Components/NPC/NpcAIComponent.h>
#include <Engine/Scene/Components/Animation/NpcAnimationControllerComponent.h>


class NpcAIMovementSystem
{
public:

	static void UpdateNPCAIMovementSystem(float deltaTime, Engine::Scene* scene);

	
private:

	static void RotateTowardsDirXY(Engine::TransformComponent& tr, const glm::vec3& dir, float dt);
	static bool HasLOSNow(const Engine::Ref<Engine::GridMap>& grid, const glm::vec3& npcPos3, const glm::vec3& targetPos3);
	static void EnterIdle(NPCAIMovementComponent& ai);
	static void EnterChaseLOS(NPCAIMovementComponent& ai);
	static void EnterMoveToLastKnown(NPCAIMovementComponent& ai);
	static void BeginOneShotAction(NPCAIMovementComponent& ai, NpcAnimationControllerComponent& animCtrl, AIState actionState, float durationSec, AIState returnState);
	static bool MoveAlongPathXY(NPCAIMovementComponent& ai, Engine::TransformComponent& tr, float dt);
};

