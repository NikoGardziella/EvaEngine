#pragma once
#include <Engine/Scene/Components/Vehicles/VehicleComponent.h>
#include <Engine/Scene/Scene.h>


class Scene;
class VehicleCollisionSystem
{
public:
	static void UpdateVehicleCollision(float deltaTime, Engine::Scene* scene);

	static void ApplyPush(Engine::TransformComponent& transform, VehicleComponent& vehicle, const glm::vec2& sourcePosition, float pushStrength = 0.2f, float velocityNudge = 0.5f);

};

