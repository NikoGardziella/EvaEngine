#include "PlayerWeaponSystem.h"
#include <Engine/Scene/Components/Combat/WeaponComponent.h>
#include <Engine/AssetManager/AssetManager.h>
#include <Engine/Debug/Instrumentor.h>
#include <Engine/Scene/Components/Vehicles/DriverComponent.h>
#include <Engine/Scene/Components/Projectiles/ProjectileComponent.h>
#include <Engine/Scene/Scene.h>
#include <Engine/Scene/Components/Render/ChunkRendererComponent.h>
#include <Engine/Core/Log.h>
#include <Engine/Scene/Components/Animation/AnimationComponent.h>
#include <Engine/Scene/Components/Render/3D/AnimatorComponent.h>
#include <Engine/Map/Projectile/ProjectileVisualRegistry.h>
#include <Engine/Scene/Components/Combat/HealthComponent.h>
#include <Engine/Scene/Components/NPC/NpcAIStateComponent.h>
#include <glm/gtc/random.hpp>
#include <Engine/Scene/Components/Combat/ThrowableComponent.h>
#include <Engine/Scene/Components/Render/3D/RenderBoundsComponent.h>
#include <Engine/Scene/Components/Render/3D/MeshRefComponent.h>
#include <Engine/Scene/Components/UI/HUDStateComponent.h>
#include <Engine/Scene/Component.h>
#include "Engine/Map/Tile/CompactTileMap.h"
#include "Engine/Map/Utils/IsoTileUtils.h"

void PlayerWeaponSystem::UpdatePlayerWeaponSystem(float deltaTime, Engine::Scene* scene)
{
    EE_PROFILE_FUNCTION();

    // Mouse world position from the primary camera
    glm::vec2 mouseWorldPosition{ 0.0f, 0.0f };
    bool hasPrimaryCamera = false;

    scene->ForEach<Engine::TransformComponent, Engine::CameraComponent>(
        [&](Engine::Entity /*cameraEntity*/,
            Engine::TransformComponent& cameraTransformComp,
            Engine::CameraComponent& cameraComp)
        {
            if (hasPrimaryCamera || !cameraComp.Primary)
                return;

            mouseWorldPosition = cameraComp.Camera.ScreenToWorld(cameraTransformComp.GetTransform());
            hasPrimaryCamera = true;
        });

    const bool fireDown = Engine::Input::IsMouseButtonPressed(Engine::Mouse::Button0);
    

    scene->ForEach<Engine::TransformComponent, WeaponComponent>(
        [&](Engine::Entity playerEntity,
            Engine::TransformComponent& playerTransformComp,
            WeaponComponent& weaponComp)
        {

            if (HUDStateComponent* hudstateComp = playerEntity.TryGetComponent<HUDStateComponent>())
            {
                hudstateComp->weaponType = playerEntity.GetComponent<WeaponComponent>().type;

                if (playerEntity.HasComponent<AmmoComponent>())
                {
                    auto& ammo = playerEntity.GetComponent<AmmoComponent>();
                    hudstateComp->ammoInMag = ammo.ammoInMag;
                    hudstateComp->magSize = ammo.magSize;
                    hudstateComp->showAmmo = true;
                }
                else
                {
                    hudstateComp->showAmmo = false;
                }


            }
            

            


            const glm::vec2 playerPos = playerTransformComp.Translation;
            const glm::vec2 dir = mouseWorldPosition - playerPos;
            weaponComp.IsAiming = false;
            weaponComp.IsFiring = fireDown;

            if (weaponComp.Cooldown > 0.0f)
            {
                weaponComp.Cooldown -= deltaTime;
            }

            if (weaponComp.type == WeaponType::Grenade)
            {
                if (fireDown && weaponComp.Cooldown <= 0.0f)
                {
                    if (!weaponComp.GrenadeIsCharging)
                    {
                        weaponComp.GrenadeIsCharging = true;
                        weaponComp.GrenadeChargeTime = 0.0f;
                    }

                    weaponComp.GrenadeChargeTime += deltaTime;

                    if (weaponComp.GrenadeChargeTime > weaponComp.GrenadeMaxCharge)
                    {
                        weaponComp.GrenadeChargeTime = weaponComp.GrenadeMaxCharge;
                    }

                }
                else
                {
                    // Button is up now
                    if (weaponComp.GrenadeIsCharging && weaponComp.Cooldown <= 0.0f)
                    {
                        // Convert charge time -> throw speed
                        float t = (weaponComp.GrenadeMaxCharge > 0.0f)
                            ? (weaponComp.GrenadeChargeTime / weaponComp.GrenadeMaxCharge)
                            : 1.0f;
                        t = glm::clamp(t, 0.0f, 1.0f);

                        float throwSpeed = glm::mix(weaponComp.GrenadeMinSpeed,
                            weaponComp.GrenadeMaxSpeed, t);

                        // Use a temp copy so we do not overwrite the base ProjectileSpeed
                        WeaponComponent temp = weaponComp;
                        temp.ProjectileSpeed = throwSpeed;

                        FireThrowableWeapon(playerEntity, playerTransformComp, mouseWorldPosition, temp, scene);

                        weaponComp.Cooldown = weaponComp.FireRate;
                    }

                    weaponComp.GrenadeIsCharging = false;
                    weaponComp.GrenadeChargeTime = 0.0f;
                }

                return;
            }

            if (!fireDown)
            {
                weaponComp.IsAiming = true;

                return;
            }


            if (weaponComp.Cooldown > 0.0f)
                return;
            weaponComp.FiredThisFrame = fireDown;

            switch (weaponComp.type)
            {
            case WeaponType::Melee:
                FireMeleeWeapon(playerEntity, playerTransformComp, weaponComp, scene);
                break;

            case WeaponType::Pistol:
            case WeaponType::MachineGun:
                FireSingleProjectileWeapon(playerEntity, playerTransformComp,
                    mouseWorldPosition, weaponComp, scene);
                break;

            case WeaponType::Shotgun:
                FireShotgunWeapon(playerEntity, playerTransformComp,
                    mouseWorldPosition, weaponComp, scene);
                break;

            case WeaponType::Bazooka:
                FireExplosiveProjectileWeapon(playerEntity, playerTransformComp,
                    mouseWorldPosition, weaponComp, scene);
                break;

            default:
                break;
            }

            weaponComp.Cooldown = weaponComp.FireRate;
        });


        




}


Engine::Entity PlayerWeaponSystem::SpawnProjectileEntity(Engine::Scene* scene, Engine::Entity owner,
    const glm::vec2& origin, const glm::vec2& direction, const WeaponComponent& weaponComp , const glm::vec2& aimedPosWorld)
{
    EE_PROFILE_FUNCTION();

    glm::vec2 dirNorm = glm::normalize(direction);

    Engine::Entity projectileEntity = scene->CreateEntity("Projectile");

    auto& transformComp = projectileEntity.AddComponent<Engine::TransformComponent>();
    transformComp.Translation = glm::vec3(origin, 0.0f);
    transformComp.Rotation.z = std::atan2(dirNorm.y, dirNorm.x);

    float projectileMaxRange = weaponComp.MaxRange;
    float projectileRadius = 0.1f;

    float distanceToAimedPosWorld = glm::distance(aimedPosWorld, origin);
    ProjectileComponent& projectileComp = projectileEntity.AddComponent<ProjectileComponent>(dirNorm, projectileMaxRange);

    projectileComp.Damage = weaponComp.Damage;
    projectileComp.ProjectileRadius = projectileRadius;
    projectileComp.DestructionRadius = weaponComp.DestructionRadius;
    projectileComp.Owner = owner;
    projectileComp.TargetPositionHeightZ1 = SampleHeightAt(scene, aimedPosWorld, 1);
    projectileComp.DistanceToTargetatFireTime = distanceToAimedPosWorld;
    projectileComp.TargetPositionAtFireTime = aimedPosWorld;
    projectileComp.ProjectileSped = weaponComp.ProjectileSpeed;
    projectileComp.renderSlot = Engine::ProjectileVisual::GetSlot(ProjectileVisualType::Bullet);
    projectileComp.DistanceTravelled = 0.0f;

    projectileComp.AffectedTileUIDs = BuildProjectileAffectedUIDs(scene, origin, aimedPosWorld, projectileRadius, weaponComp.DestructionRadius);

    return projectileEntity;
}

void PlayerWeaponSystem::FireShotgunWeapon(Engine::Entity player, Engine::TransformComponent& transformComp,
    const glm::vec2& mouseWorld, const WeaponComponent& weaponComp, Engine::Scene* scene)
{
    EE_PROFILE_FUNCTION();

    const glm::vec2 origin = glm::vec2(transformComp.Translation);

    glm::vec2 baseDir = mouseWorld - origin;
    if (glm::length2(baseDir) < 0.0001f)
        return;

    baseDir = glm::normalize(baseDir);

    // Distance from player to where the player aimed
    const float aimedDist = glm::length(mouseWorld - origin);

    // Clamp to weapon max range if have one
    const float maxRange = weaponComp.MaxRange;
    float travelDist = aimedDist;
    if (maxRange > 0.0f && travelDist > maxRange)
    {
        travelDist = maxRange;
    }

    const uint32_t pellets = glm::max(weaponComp.Pellets, 1u);
    const float spreadRad = glm::radians(weaponComp.SpreadDegrees);

    for (uint32_t i = 0; i < pellets; ++i)
    {
        glm::vec2 dir = baseDir;
        if (spreadRad > 0.0f)
        {
            const float offset = glm::linearRand(-spreadRad * 0.5f, spreadRad * 0.5f);
            const float c = std::cos(offset);
            const float s = std::sin(offset);
            dir = glm::vec2(
                baseDir.x * c - baseDir.y * s,
                baseDir.x * s + baseDir.y * c
            );
        }

        glm::vec2 perp = glm::vec2(-baseDir.y, baseDir.x);
        float backOffset = glm::linearRand(-0.05f, 0.2f);
        float sideOffset = glm::linearRand(-0.08f, 0.08f);
        glm::vec2 pelletOrigin = origin - baseDir * backOffset + perp * sideOffset;

        const glm::vec2 pelletEndWorld = pelletOrigin + dir * travelDist;
        SpawnProjectileEntity(scene, player, pelletOrigin, dir, weaponComp, pelletEndWorld);
    }
}



void PlayerWeaponSystem::FireExplosiveProjectileWeapon(Engine::Entity player, Engine::TransformComponent& tr,
    const glm::vec2& mouseWorld, const WeaponComponent& weapon, Engine::Scene* scene)
{
    EE_PROFILE_FUNCTION();

    const glm::vec2 origin = glm::vec2(tr.Translation);
    glm::vec2 dir = mouseWorld - origin;
    if (glm::length2(dir) < 0.0001f)
        return;

    dir = glm::normalize(dir);

    SpawnProjectileEntity(scene, player, origin, dir, weapon, mouseWorld);
}

void PlayerWeaponSystem::FireMeleeWeapon(Engine::Entity player, Engine::TransformComponent& transformComp,
    const WeaponComponent& weaponComp, Engine::Scene* scene)
{
    EE_PROFILE_FUNCTION();
    const glm::vec2 origin = glm::vec2(transformComp.Translation);
    const float range = weaponComp.MeleeRange;
    const float halfArc = glm::radians(weaponComp.MeleeArcDegrees * 0.5f);

    // Facing direction from player rotation (assuming z is yaw)
    glm::vec2 forward(std::cos(transformComp.Rotation.z), std::sin(transformComp.Rotation.z));

    // Iterate potential targets (NPCs)
    scene->ForEach<Engine::TransformComponent, NpcAIStateComponent, HealthComponent>(
        [&](Engine::Entity npc, Engine::TransformComponent& npcTr,  NpcAIStateComponent& npcState,
            HealthComponent& health)
        {
            glm::vec2 toTarget = glm::vec2(npcTr.Translation) - origin;
            float dist2 = glm::length2(toTarget);
            if (dist2 > range * range)
                return;

            float dist = std::sqrt(dist2);
            glm::vec2 dir = toTarget / dist;

            float angle = std::atan2(
                forward.x * dir.y - forward.y * dir.x, // cross
                forward.x * dir.x + forward.y * dir.y  // dot
            );

            if (std::abs(angle) <= halfArc)
            {
                // Hit!
                health.Current -= weaponComp.Damage;
            }
        }
    );
}

void PlayerWeaponSystem::FireSingleProjectileWeapon(Engine::Entity player, Engine::TransformComponent& transformComp,
    const glm::vec2& mouseWorld, const WeaponComponent& weaponComp, Engine::Scene* scene)
{
    EE_PROFILE_FUNCTION();

    const glm::vec2 origin = glm::vec2(transformComp.Translation);

    glm::vec2 dir = mouseWorld - origin;
    if (glm::length2(dir) < 0.0001f)
        return;

    dir = glm::normalize(dir);

    const float spreadRad = glm::radians(weaponComp.SpreadDegrees);
    if (spreadRad > 0.0f)
    {
        const float offset = glm::linearRand(-spreadRad * 0.5f, spreadRad * 0.5f);
        const float c = std::cos(offset);
        const float s = std::sin(offset);

        dir = glm::vec2(
            dir.x * c - dir.y * s,
            dir.x * s + dir.y * c
        );
    }

    // Distance from player to where they aimed
    const float aimedDist = glm::length(mouseWorld - origin);

    // Clamp to weapon max range if specified
    const float maxRange = weaponComp.MaxRange;
    float travelDist = aimedDist;
    if (maxRange > 0.0f && travelDist > maxRange)
    {
        travelDist = maxRange;
    }

    // Correct world-space end position along final dir
    const glm::vec2 endWorld = origin + dir * travelDist;

    SpawnProjectileEntity(scene, player, origin, dir, weaponComp, endWorld);
}

void PlayerWeaponSystem::FireThrowableWeapon(Engine::Entity player, Engine::TransformComponent& transformComp,
    const glm::vec2& mouseWorld, const WeaponComponent& weaponComp, Engine::Scene* scene)
{
    const glm::vec2 origin = glm::vec2(transformComp.Translation);
    glm::vec2 dir = mouseWorld - origin;
    float dist = glm::length(dir);
    if (dist < 0.0001f)
        return;

    glm::vec2 dirNorm = dir / dist;

    // --- Spawn 3D grenade entity ---
    Engine::MeshRegistry& meshReg = Engine::AssetManager::GetMeshRegistry();
    const Engine::MeshAsset* meshAsset = meshReg.GetMeshByKey("nade_low");
    EE_CORE_ASSERT(meshAsset, "nade_low mesh not found");

    Engine::Entity grenade = scene->CreateEntity("Grenade3D");

    auto& grenadeTr = grenade.AddComponent<Engine::TransformComponent>();
    grenadeTr.Translation = glm::vec3(origin, 0.0f);
    grenadeTr.Rotation = glm::vec3(0.0f);  // you'll drive Z in ThrowableSystem
    grenadeTr.Scale = glm::vec3(1.0f);

    auto& rb = grenade.AddComponent<Engine::RenderBoundsComponent>();
    rb.minL = meshAsset->minL;
    rb.maxL = meshAsset->maxL;

    // Mesh
    auto& MeshRefComp = grenade.AddComponent<Engine::MeshRefComponent>();
    MeshRefComp.meshId = meshAsset->id;
    MeshRefComp.submeshCount = meshAsset->submeshes.size();
    MeshRefComp.submeshFirst = 0;
    // Optional: if you use per-entity override material (otherwise rely on mesh submesh materials)
    // auto& matRef = grenade.AddComponent<Engine::MaterialRefComponent>();
    // matRef.materialId = weaponComp.GrenadeMaterialId; // if you have this

    // --- Throwable sim ---
    auto& throwableComp = grenade.AddComponent<ThrowableComponent>();
    throwableComp.Type = ThrowableType::Grenade;

    throwableComp.GroundPosWS = origin;

    const float v = weaponComp.ProjectileSpeed;
    throwableComp.InitialSpeed = v;
    throwableComp.VelocityWS = dirNorm * v;

    throwableComp.FuseTime = 2.0f;
    throwableComp.FuseTimer = throwableComp.FuseTime;

    float speed01 = 0.0f;
    {
        const float minV = weaponComp.GrenadeMinSpeed;
        const float maxV = weaponComp.GrenadeMaxSpeed;
        if (maxV > minV)
            speed01 = glm::clamp((v - minV) / (maxV - minV), 0.0f, 1.0f);
    }

    const float minHeight = 0.4f;
    const float maxHeight = 1.2f;
    float lowSpeedFactor = 1.0f - speed01;
    throwableComp.ArcHeightWorld = glm::mix(minHeight, maxHeight, lowSpeedFactor);

    const float minLift = 0.5f;
    const float maxLift = 0.6f;
    throwableComp.InitialLift = glm::mix(minLift, maxLift, lowSpeedFactor);

    const float minArcTime = 0.3f;
    const float maxArcTime = 0.9f;
    float travelTime = (v > 0.0f) ? (dist / v) : 0.5f;
    throwableComp.ArcDuration = glm::clamp(travelTime, minArcTime, maxArcTime);

    throwableComp.MinSpeedToStop = glm::mix(0.5f, 2.0f, speed01);
    throwableComp.Bounciness = glm::mix(0.3f, 0.6f, speed01);

    throwableComp.AirDrag = 1.5f;

    throwableComp.MaxBounces = 2;
    throwableComp.ExplodeOnImpact = false;

    throwableComp.Damge = weaponComp.Damage;
    throwableComp.DestructionRadius = weaponComp.DestructionRadius;

    throwableComp.RotationZ = 0.0f;
    throwableComp.AngularSpeedZ = glm::mix(throwableComp.MinSpinSpeed, throwableComp.MaxSpinSpeed, speed01);

  
}





inline glm::ivec2 WorldToCell(const glm::vec2& world)
{
    return glm::ivec2(
        static_cast<int>(std::round(world.x)),
        static_cast<int>(std::round(world.y))
    );
}
std::vector<uint64_t> PlayerWeaponSystem::BuildProjectileAffectedUIDs(
    Engine::Scene* scene,
    const glm::vec2& origin,
    const glm::vec2& target,
    float projectileRadius,
    float destructionRadius)
{
    EE_PROFILE_FUNCTION();

    std::vector<uint64_t> resultUIDs;

    if (!scene)
        return resultUIDs;

    Engine::CompactTileMap& compactMap = scene->GetCompactTileMap();

    std::unordered_set<uint64_t> uniqueUIDs;
    std::unordered_set<int64_t> visitedCells;

    glm::vec2 delta = target - origin;
    float length = glm::length(delta);

    if (length <= 0.0001f)
        return resultUIDs;

    glm::vec2 dir = delta / length;

    // Step less than one tile, so we do not skip cells.
    const float stepWorld = float(TILE_SIZE) * 0.25f;
    const int steps = std::max(1, int(std::ceil(length / stepWorld)));

    auto CellKey = [](const glm::ivec2& c) -> int64_t
        {
            return (int64_t(c.x) << 32) ^ uint32_t(c.y);
        };

    auto AddCell = [&](const glm::ivec2& cell)
        {
            int64_t key = CellKey(cell);

            if (!visitedCells.insert(key).second)
                return;

            std::vector<Engine::CompactTile>* compactTiles = compactMap.GetTiles(cell);
            if (!compactTiles)
                return;

            for (const Engine::CompactTile& tile : *compactTiles)
            {
                const uint64_t uid = tile.UID;

                if (uid == 0)
                    continue;

                uniqueUIDs.insert(uid);
            }
        };

    for (int i = 0; i <= steps; i++)
    {
        float t = float(i) / float(steps);
        glm::vec2 p = origin + delta * t;

        glm::ivec2 cell = Engine::IsoTileUtils::WorldToIsoCellInt(p);

        AddCell(cell);

        // Small safety around the ray.
        AddCell(cell + glm::ivec2(1, 0));
        AddCell(cell + glm::ivec2(-1, 0));
        AddCell(cell + glm::ivec2(0, 1));
        AddCell(cell + glm::ivec2(0, -1));
    }

    resultUIDs.reserve(uniqueUIDs.size());

    for (uint64_t uid : uniqueUIDs)
        resultUIDs.push_back(uid);
    return resultUIDs;
}

float SampleHeightAt_FromTileCenterFudged(
    const glm::vec2& worldXY,
    const glm::vec2& tileCenter,
    const glm::vec2& tileSizeWorld,
    float pxWorld,
    const glm::vec2& originBiasWorld = glm::vec2(0.5f, 0.0f), 
    bool snapToTexelCenters = true)
{
    // nominal TL from center (Y up)
    const glm::vec2 originTL_nominal = tileCenter + glm::vec2(-0.5f * tileSizeWorld.x,
        +0.5f * tileSizeWorld.y);

    // apply the same bias you used when pushing to compute (center - bias)
    const glm::vec2 originTL = originTL_nominal - originBiasWorld;

    // bottom edge in world
    const float bottomY = originTL.y - tileSizeWorld.y;

    // raw height above bottom
    float h = worldXY.y - bottomY;

    // optional: snap to texel centers to avoid half-texel drift
    if (snapToTexelCenters && pxWorld > 0.0f)
    {
        float rows = h / pxWorld;
        rows = std::floor(rows) + 0.5f; // center of the row
        h = rows * pxWorld;
    }

    // clamp to the tile’s vertical span
    if (h < 0.0f) h = 0.0f;
    if (h > tileSizeWorld.y) h = tileSizeWorld.y;
    return h;
}

inline float clampf(float v, float a, float b) { return v < a ? a : (v > b ? b : v); }

float SampleHeightAt_FromBottomLeft(
    const glm::vec2& worldXY,
    const glm::vec2& originBL,          // bottom-left of the tile
    float tileWorldH,
    float pxWorldY,                      // world units per texel row
    bool snapToTexelCenters = true)
{
    // height above bottom edge
    float h = worldXY.y - originBL.y;

    if (snapToTexelCenters && pxWorldY > 0.0f) {
        float rows = h / pxWorldY;
        rows = std::floor(rows) + 0.5f; // center of the row
        h = rows * pxWorldY;
    }

    return clampf(h, 0.0f, tileWorldH);
}

float PlayerWeaponSystem::SampleHeightAt(Engine::Scene* scene, const glm::vec2& worldXY, int /*radiusPx*/)
{
    // World-units per pixel (separate X/Y!)
    const float pxWorldX = float(TILE_SIZE) / float(TILE_PIXEL_WIDTH);
    const float pxWorldY = float(TILE_SIZE) / float(TILE_PIXEL_HEIGHT);

    const float tileWorldW = float(TILE_PIXEL_WIDTH) * pxWorldX;
    const float tileWorldH = float(TILE_PIXEL_HEIGHT) * pxWorldY;

    bool  hit = false;
    float best = 0.0f;

    scene->ForEachConst<Engine::TransformComponent, Engine::TileComponent>(
        [&](Engine::Entity, const Engine::TransformComponent& tr, const Engine::TileComponent& tc)
        {
            for (const Engine::TileInfo& t : tc.tiles)
            {
                // correct center (don’t overwrite it!)
                glm::vec2 tileCenter = glm::vec2(tr.Translation) + t.position;

                glm::vec2 originBL = tileCenter - 0.5f * glm::vec2(tileWorldW, tileWorldH);
                glm::vec2 minW = originBL;
                glm::vec2 maxW = originBL + glm::vec2(tileWorldW, tileWorldH);

                // open max-edges to avoid double-ownership on borders
                if (worldXY.x < minW.x || worldXY.x >= maxW.x ||
                    worldXY.y < minW.y || worldXY.y >= maxW.y)
                    continue;

                float h = SampleHeightAt_FromBottomLeft(worldXY, originBL, tileWorldH, pxWorldY, /*snap*/true);

                if (!hit || h > best) { best = h; hit = true; }
            }
        });

    return hit ? best : 0.0f;
}
