#pragma once
#include <Engine/Scene/Components/Vehicles/VehicleComponent.h>
#include <Engine/Scene/Scene.h>
#include <Engine/Map/Grid/GridUtils/GridUtils.h>

#include "CollisionSystemUtils.h"
#include <vector>
#include "glm/glm.hpp"
#include <Engine/Scene/Component.h>

class Scene;
class VehicleCollisionSystem
{
public:

   

public:
	static void UpdateVehicleCollision(float deltaTime, Engine::Scene* scene);


private:
    static CollisionSystemUtils::CollisionMoveResult CollideAndSlideVehicleBox(const std::vector<Engine::SubCellOBB>& walls, glm::vec2 pos, glm::vec2 delta, glm::vec2 vehicleHalfExtents);
	static std::vector<uint64_t> BuildVehicleAffectedUIDs(Engine::Scene* scene, const glm::vec2& center, const glm::vec2& halfExtents, float rotationRadians);

	static void ApplyPush(Engine::TransformComponent& transform, VehicleComponent& vehicle, const glm::vec2& sourcePosition, float pushStrength = 0.2f, float velocityNudge = 0.5f);

};

