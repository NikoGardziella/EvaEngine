#pragma once
#include "entt.hpp"
#include "Engine.h"
#include <Engine/Scene/Components/Vehicles/VehicleComponent.h>

class VehicleCollisionSystem
{
public:
	static void UpdateVehicleCollision(entt::registry& registry, float deltaTime, Engine::Scene* scene);

	static void ApplyPush(Engine::TransformComponent& transform, VehicleComponent& vehicle, const glm::vec2& sourcePosition, float pushStrength = 0.2f, float velocityNudge = 0.5f);

};

