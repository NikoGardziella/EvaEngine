#pragma once
#include <Engine/Scene/Scene.h>
#include <Engine/Scene/Components/NPC/NpcAIComponent.h>
#include "glm/glm.hpp"

class Scene;
class NpcAIStateSystem
{
	public:
		static void NpcAIStateSystem::UpdateNpcAIStateSystem(float dt, Engine::Scene* scene);

		static void SetRandomPatrolGoal(NPCAIMovementComponent& mv, const glm::vec3& npcPos, float minRadius, float maxRadius, bool usePath);

	private:

};

