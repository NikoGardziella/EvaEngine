#pragma once

#include <Engine/Map/Grid/GridMap.h>
#include "Engine/Map/Grid/GridUtils/GridUtils.h"
#include <Engine/Scene/Scene.h>
#include "CollisionSystemUtils.h"


class Scene;
class PlayerCollisionSystem
{
private:
   
    
   

   
public:
    static void UpdatePlayerCollision(float deltaTime, Engine::Scene* scene);
private:
    static CollisionSystemUtils::SweepHit SweepCircleVsOBB(const Engine::SubCellOBB& obb, const glm::vec2& p0, const glm::vec2& delta, float radius, float skin);

    static glm::vec2 CollideAndSlideOBBs(const std::vector<Engine::SubCellOBB>& walls, glm::vec2 pos, glm::vec2 delta, float radius);

 

    



   

    
   
};
