#include "VehicleCollisionSystem.h"
#include <Engine/Scene/Scene.h>

#include <Engine/Debug/Instrumentor.h>
#include <Engine/Events/Public/CollisionEvents.h>
#include <Engine/Map/Grid/GridUtils/GridUtils.h>
#include <Engine/Scene/Scene.h>
#include <Engine/Map/Grid/GridMap.h>
#include "Engine/Renderer/Renderer2D/VulkanRenderer2D.h"
#include <Engine/Map/Utils/IsoTileUtils.h>
#include "Engine/AssetManager/AssetManager.h"
#include "Engine/Math/HashUtils.h"

void VehicleCollisionSystem::UpdateVehicleCollision(float deltaTime, Engine::Scene* scene)
{
    EE_PROFILE_FUNCTION();

    if (!scene)
        return;

    const std::vector<Engine::SubCellOBB>& walls = scene->GetGrid()->GetGridSubcells();

    scene->ForEach<Engine::TransformComponent, VehicleComponent, Engine::IDComponent>(
        [&](Engine::Entity vehicleEntity, Engine::TransformComponent& vehicleTransformComp,
            VehicleComponent& vehicleComp, Engine::IDComponent& carIDComponent)
        {
            glm::vec2 pos = glm::vec2(vehicleTransformComp.Translation);

            glm::vec2 velocity = vehicleComp.Velocity;

            if (glm::length2(velocity) < 1e-8f)
                return;

            glm::vec2 delta = velocity * deltaTime;

            
            glm::vec2 vehicleHalfExtents = glm::vec2(0.45f, 0.25f);

            CollisionSystemUtils::CollisionMoveResult result = CollideAndSlideVehicleBox(
                walls,
                pos,
                delta,
                vehicleHalfExtents
            );



            vehicleTransformComp.Translation.x = result.FinalPosition.x;
            vehicleTransformComp.Translation.y = result.FinalPosition.y;

           // if (!result.Hit)
           //     return;

            float frontWidth = vehicleHalfExtents.x * 2.0f;
            float destructionRadius = frontWidth * 0.5f;

            float speed = glm::length(vehicleComp.Velocity);

            // Usually kinetic energy has 0.5f, but you can tune this.
            float impactEnergy = 0.5f * vehicleComp.Mass * speed * speed;

            uint32_t damage = static_cast<uint32_t>(
                glm::clamp(impactEnergy * 0.05f, 1.0f, 255.0f)
                );

            vehicleComp.CollisionCooldown -= deltaTime;

            if (result.Hit)
            {
                bool cellDestroyed = false;
                bool didDamageThisHit = false;

                if (vehicleComp.CollisionCooldown <= 0.0f)
                {
                    for (uint64_t key : result.HitSubCellKeys)
                    {
                        cellDestroyed |= scene->GetGrid()->DamageSubCell(key, damage);
                    }

                    scene->GetGrid()->RemoveDeadSubCells();

                    glm::vec2 damagePoint = result.HitPoint - result.HitNormal * 0.20f;

                    std::vector<uint64_t> affectedUIDs = BuildVehicleAffectedUIDs(
                        scene,
                        damagePoint,
                        vehicleHalfExtents,
                        vehicleTransformComp.Rotation.z
                    );

                    Engine::VulkanRenderer2D::CalculateBoxCollision(
                        damagePoint,
                        glm::vec2(1.0f, 2.0f),
                        vehicleTransformComp.Rotation.z,
                        carIDComponent.ID,
                        Engine::eCollisionType::VEHICLE,
                        damage,
                        affectedUIDs
                    );

                    vehicleComp.CollisionCooldown = 0.5f;
                    didDamageThisHit = true;
                }

                vehicleComp.CurrentSpeed *= 0.3f;

                const float kPushbackStrength = 0.8f;
                const float kVelocityNudge = 0.3f;

                if (!cellDestroyed)
                {
                    ApplyPush(
                        vehicleTransformComp,
                        vehicleComp,
                        result.HitPoint,
                        kPushbackStrength,
                        kVelocityNudge
                    );
                }
            }

          


         
            
        });
}
std::vector<uint64_t> VehicleCollisionSystem::BuildVehicleAffectedUIDs(Engine::Scene* scene, const glm::vec2& center, const glm::vec2& halfExtents, float rotationRadians)
{
    EE_PROFILE_FUNCTION();

    std::vector<uint64_t> result;

    if (!scene)
        return result;

    const float c = std::cos(rotationRadians);
    const float s = std::sin(rotationRadians);

    auto Rotate = [&](glm::vec2 p)
        {
            return glm::vec2(
                p.x * c - p.y * s,
                p.x * s + p.y * c
            );
        };

    glm::vec2 corners[4] =
    {
        center + Rotate({ -halfExtents.x, -halfExtents.y }),
        center + Rotate({  halfExtents.x, -halfExtents.y }),
        center + Rotate({  halfExtents.x,  halfExtents.y }),
        center + Rotate({ -halfExtents.x,  halfExtents.y })
    };

    glm::vec2 minW = corners[0];
    glm::vec2 maxW = corners[0];

    for (int i = 1; i < 4; i++)
    {
        minW = glm::min(minW, corners[i]);
        maxW = glm::max(maxW, corners[i]);
    }

    minW -= glm::vec2(0.25f);
    maxW += glm::vec2(0.25f);

    glm::ivec2 c0 = Engine::IsoTileUtils::WorldToIsoCellInt({ minW.x, minW.y });
    glm::ivec2 c1 = Engine::IsoTileUtils::WorldToIsoCellInt({ maxW.x, minW.y });
    glm::ivec2 c2 = Engine::IsoTileUtils::WorldToIsoCellInt({ minW.x, maxW.y });
    glm::ivec2 c3 = Engine::IsoTileUtils::WorldToIsoCellInt({ maxW.x, maxW.y });

    glm::ivec2 minCell{
        std::min(std::min(c0.x, c1.x), std::min(c2.x, c3.x)),
        std::min(std::min(c0.y, c1.y), std::min(c2.y, c3.y))
    };

    glm::ivec2 maxCell{
        std::max(std::max(c0.x, c1.x), std::max(c2.x, c3.x)),
        std::max(std::max(c0.y, c1.y), std::max(c2.y, c3.y))
    };

    minCell -= glm::ivec2(1);
    maxCell += glm::ivec2(1);

    Engine::CompactTileMap& compactMap = scene->GetCompactTileMap();
    Engine::TileDefinitionRegistry& defs = Engine::AssetManager::GetTileDefinitions();

    std::unordered_set<uint64_t> uniqueUIDs;

    for (int y = minCell.y; y <= maxCell.y; y++)
    {
        for (int x = minCell.x; x <= maxCell.x; x++)
        {
            glm::ivec2 cell{ x, y };

            std::vector<Engine::CompactTile>* compactTiles = compactMap.GetTiles(cell);
            if (!compactTiles)
                continue;

            for (const Engine::CompactTile& tile : *compactTiles)
            {
                if (tile.IsEmpty())
                    continue;

                const Engine::TileDefinition* def = defs.Get(tile.TypeId);
                if (!def)
                    continue;

                const Engine::CompactGroupInfo* groupInfo =
                    compactMap.GetGroupInfo(tile.GroupId);

                if (!groupInfo)
                    continue;

                const glm::vec2 tileWorldPos = Engine::IsoTileUtils::IsoToWorldGround(cell);
                const glm::vec2 rootWorldPos = Engine::IsoTileUtils::IsoToWorldGround(groupInfo->OriginCell);
                const glm::vec2 localPos = tileWorldPos - rootWorldPos;

                const uint64_t uid = HashUtils::MakeTileUID(
                    tile.GroupId, // same as id.ID, because promoted entity ID is set to groupId
                    localPos,
                    float(TILE_SIZE),
                    static_cast<uint32_t>(def->Category),
                    def->Direction
                );

                uniqueUIDs.insert(uid);
            }
        }
    }

    result.reserve(uniqueUIDs.size());

    for (uint64_t uid : uniqueUIDs)
        result.push_back(uid);

    return result;
}


CollisionSystemUtils::CollisionMoveResult VehicleCollisionSystem::CollideAndSlideVehicleBox(
    const std::vector<Engine::SubCellOBB>& walls,
    glm::vec2 pos,
    glm::vec2 delta,
    glm::vec2 vehicleHalfExtents)
{
    CollisionSystemUtils::CollisionMoveResult result{};
    result.FinalPosition = pos;

    if (glm::length2(delta) < 1e-12f)
        return result;

    glm::vec2 rem = delta;

    const float skin = 1e-3f;
    const int maxIters = 4;

    auto AddHitKey = [&](uint64_t key)
        {
            if (key == 0)
                return;

            if (std::find(
                result.HitSubCellKeys.begin(),
                result.HitSubCellKeys.end(),
                key) == result.HitSubCellKeys.end())
            {
                result.HitSubCellKeys.push_back(key);
            }
        };

    for (int iter = 0; iter < maxIters; ++iter)
    {
        const CollisionSystemUtils::AABB2 sweptAABB =
            CollisionSystemUtils::MakeSweptAABB(
                pos,
                rem,
                glm::length(vehicleHalfExtents)
            );

        CollisionSystemUtils::SweepHit bestDynamic{};
        uint64_t bestDynamicKey = 0;

        bool hasStaticPush = false;
        uint64_t staticPushKey = 0;

        glm::vec2 staticPushPos = pos;
        glm::vec2 staticPushNormal(0.0f);
        float staticPushDist2 = 0.0f;

        for (int wallIndex = 0; wallIndex < (int)walls.size(); ++wallIndex)
        {
            const Engine::SubCellOBB& wall = walls[wallIndex];

            Engine::SubCellOBB expanded = wall;
            expanded.halfExtents += vehicleHalfExtents;

            const CollisionSystemUtils::AABB2 wallAABB =
                CollisionSystemUtils::MakeOBBAABB(expanded);

            if (!CollisionSystemUtils::Overlaps(sweptAABB, wallAABB))
                continue;

            CollisionSystemUtils::SweepHit h =
                CollisionSystemUtils::SweepCircleVsOBB(
                    expanded,
                    pos,
                    rem,
                    0.0f,
                    skin
                );

            if (!h.hit)
                continue;

            AddHitKey(wall.CollisionKey);

            if (h.toi == 0.0f && glm::length2(h.normal) > 0.0f)
            {
                glm::vec2 pushVec = h.point - pos;
                float d2 = glm::length2(pushVec);

                if (!hasStaticPush || d2 > staticPushDist2)
                {
                    hasStaticPush = true;
                    staticPushKey = wall.CollisionKey;
                    staticPushPos = h.point;
                    staticPushNormal = h.normal;
                    staticPushDist2 = d2;
                }

                continue;
            }

            if (!bestDynamic.hit || h.toi < bestDynamic.toi)
            {
                bestDynamic = h;
                bestDynamicKey = wall.CollisionKey;
            }
        }

        if (bestDynamic.hit)
        {
            result.Hit = true;
            result.HitPoint = bestDynamic.point;
            result.HitNormal = bestDynamic.normal;
            AddHitKey(bestDynamicKey);

            float tMove = std::max(0.0f, bestDynamic.toi - 1e-4f);
            pos += rem * tMove;

            glm::vec2 leftover = rem * (1.0f - tMove);

            float vn = glm::dot(leftover, bestDynamic.normal);
            if (vn < 0.0f)
                leftover -= bestDynamic.normal * vn;

            pos += bestDynamic.normal * skin;

            if (glm::length2(leftover) < 1e-10f)
                break;

            rem = leftover;
            continue;
        }

        if (hasStaticPush)
        {
            result.Hit = true;
            result.HitPoint = staticPushPos;
            result.HitNormal = staticPushNormal;
            AddHitKey(staticPushKey);

            pos = staticPushPos;

            float vn = glm::dot(rem, staticPushNormal);
            if (vn < 0.0f)
                rem -= staticPushNormal * vn;

            if (glm::length2(rem) < 1e-10f)
                break;

            continue;
        }

        pos += rem;
        break;
    }

    result.FinalPosition = pos;
    return result;
}

void VehicleCollisionSystem::ApplyPush(
    Engine::TransformComponent& transform,
    VehicleComponent& vehicle,
    const glm::vec2& sourcePosition,
    float basePushStrength,
    float baseVelocityNudge
)
{
    glm::vec2 vehiclePos = glm::vec2(transform.Translation);
    glm::vec2 pushDir = glm::normalize(vehiclePos - sourcePosition);

    if (!glm::any(glm::isnan(pushDir)))
    {
      
        float scaledPushStrength = basePushStrength * vehicle.CurrentSpeed;
        float scaledVelocityNudge = baseVelocityNudge * vehicle.CurrentSpeed;

        vehicle.Pushback += pushDir * scaledPushStrength;
        vehicle.Velocity += pushDir * scaledVelocityNudge;

        vehicle.CurrentSpeed *= 0.7f;
    }
}


