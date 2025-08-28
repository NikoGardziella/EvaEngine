#pragma once



namespace Engine { class Scene; }
class VehicleSystem
{
public:
	static void UpdateVehicleSystem(float deltaTime, Engine::Scene* scene);
};

