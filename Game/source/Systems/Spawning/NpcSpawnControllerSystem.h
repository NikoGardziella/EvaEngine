#pragma once
#include <Engine/Scene/Scene.h>


class NpcSpawnControllerSystem
{
public:
	static void UpdateNpcSpawnControllerSystem(float dt, Engine::Scene* scene);

private:
	static Engine::Entity SpawnZombie(Engine::Scene* scene, uint32_t prefabID, const glm::vec2& pos);
};

