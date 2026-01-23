#pragma once

#include <Engine/Scene/Scene.h>
#include <Engine/Scene/Components/NPC/NpcAIComponent.h>
#include <Engine/Scene/Components/Animation/NpcAnimationControllerComponent.h>

class Scene;
class NpcAIMovementSystem
{
public:

	static void UpdateNPCAIMovementSystem(float deltaTime, Engine::Scene* scene);

private:
	static void RotateTowardsDirXY(Engine::TransformComponent& tr, NPCAIMovementComponent& movementComp, const glm::vec3& dir, float dt);
	static bool MoveAlongPathXY(NPCAIMovementComponent& ai, Engine::TransformComponent& tr, float dt);

	


	
};

