#pragma once


namespace Engine { class Scene; }

class PlayerCameraSystem
{
public:

	static void UpdatePlayerCameraSystem(float deltaTime, Engine::Scene* scene);
};

