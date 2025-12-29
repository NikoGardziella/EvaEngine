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


void PlayerWeaponSystem::UpdatePlayerWeaponSystem(float deltaTime,  Engine::Scene* scene)
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
            {
                return;
            }
            mouseWorldPosition = cameraComp.Camera.ScreenToWorld(cameraTransformComp.GetTransform());
            hasPrimaryCamera = true;
        });

    scene->ForEach<Engine::TransformComponent, WeaponComponent>(
        [&](Engine::Entity playerEntity,
            Engine::TransformComponent& playerTransformComp,
            WeaponComponent& weaponComp)
        {
            const glm::vec2 playerPos = playerTransformComp.Translation;
            const glm::vec2 dir = mouseWorldPosition - playerPos;

            if (weaponComp.Cooldown > 0.0f)
                weaponComp.Cooldown -= deltaTime;

            const bool firePressed = Engine::Input::IsMouseButtonPressed(Engine::Mouse::Button0);

            if (firePressed && weaponComp.Cooldown <= 0.0f)
            {
                switch (weaponComp.type)
                {
                case WeaponType::Melee:
                    FireMeleeWeapon(playerEntity, playerTransformComp, weaponComp, scene);
                    break;

                case WeaponType::Pistol:
                case WeaponType::MachineGun:
                    FireSingleProjectileWeapon(playerEntity, playerTransformComp, mouseWorldPosition, weaponComp, scene);
                    break;

                case WeaponType::Shotgun:
                    FireShotgunWeapon(playerEntity, playerTransformComp, mouseWorldPosition, weaponComp, scene);
                    break;

                case WeaponType::Grenade:
                case WeaponType::Bazooka:
                    FireExplosiveProjectileWeapon(playerEntity, playerTransformComp, mouseWorldPosition, weaponComp, scene);
                    break;
                }

                weaponComp.Cooldown = weaponComp.FireRate;
            }
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
    // Extra fields for explosives if you want:
   // projectileComp.Explosive = weaponComp.Explosive;
   // projectileComp.ExplosionRadius = weaponComp.ExplosionRadius;

    return projectileEntity;
}

void PlayerWeaponSystem::FireShotgunWeapon(
    Engine::Entity player,
    Engine::TransformComponent& transformComp,
    const glm::vec2& mouseWorld,
    const WeaponComponent& weaponComp,
    Engine::Scene* scene)
{
    EE_PROFILE_FUNCTION();

    const glm::vec2 origin = glm::vec2(transformComp.Translation);

    glm::vec2 baseDir = mouseWorld - origin;
    if (glm::length2(baseDir) < 0.0001f)
        return;

    baseDir = glm::normalize(baseDir);

    // Distance from player to where the player aimed
    const float aimedDist = glm::length(mouseWorld - origin);

    // Clamp to weapon max range if you have one
    const float maxRange = weaponComp.MaxRange;
    const float travelDist = (maxRange > 0.0f)
        ? glm::min(aimedDist, maxRange)
        : aimedDist;

    const uint32_t pellets = glm::max(weaponComp.Pellets, 1u);
    const float spreadRad = glm::radians(weaponComp.SpreadDegrees);

    for (uint32_t i = 0; i < pellets; ++i)
    {
        glm::vec2 dir = baseDir;

        if (spreadRad > 0.0f)
        {
            // Random angle offset in [-spread/2, spread/2]
            const float offset = glm::linearRand(-spreadRad * 0.5f, spreadRad * 0.5f);
            const float c = std::cos(offset);
            const float s = std::sin(offset);

            dir = glm::vec2(
                baseDir.x * c - baseDir.y * s,
                baseDir.x * s + baseDir.y * c
            );
        }

        // Correct end position in world space for this pellet
        const glm::vec2 pelletEndWorld = origin + dir * travelDist;

        // Pass pelletEndWorld as the aimed/target position instead of raw mouseWorld
        SpawnProjectileEntity(scene, player, origin, dir, weaponComp, pelletEndWorld);
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
                // optionally: spawn hit fx, sound, etc.
            }
        }
    );
}

void PlayerWeaponSystem::FireSingleProjectileWeapon(
    Engine::Entity player,
    Engine::TransformComponent& transformComp,
    const glm::vec2& mouseWorld,
    const WeaponComponent& weaponComp,
    Engine::Scene* scene)
{
    EE_PROFILE_FUNCTION();

    const glm::vec2 origin = glm::vec2(transformComp.Translation);

    // Base direction from player -> aim point
    glm::vec2 dir = mouseWorld - origin;
    if (glm::length2(dir) < 0.0001f)
        return;

    dir = glm::normalize(dir);

    // Apply spread (if any)
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
    const float travelDist = (maxRange > 0.0f)
        ? glm::min(aimedDist, maxRange)
        : aimedDist;

    // Correct world-space end position along final dir
    const glm::vec2 endWorld = origin + dir * travelDist;

    // Pass endWorld instead of raw mouseWorld
    SpawnProjectileEntity(scene, player, origin, dir, weaponComp, endWorld);
}





void PlayerWeaponSystem::ShootProjectile(Engine::Entity entity, const glm::vec2& playerPosition, const glm::vec2& mouseWorldPosition, Engine::Scene* scene, const WeaponComponent& weaponComp)
{
    EE_PROFILE_FUNCTION();

    // disable shooting from a car now
    if (entity.HasComponent<DriverComponent>())
        return;

    glm::vec2 direction = glm::normalize(mouseWorldPosition - playerPosition);
    Engine::Entity& projectileEntity = scene->CreateEntity("Projectile");

    Engine::TransformComponent& transformComp = projectileEntity.AddComponent<Engine::TransformComponent>();
	float projectileMaxRange = 1.0f; // fix this
    float projectileRadius = 0.1f;
    

   

    ProjectileComponent& projectileComp = projectileEntity.AddComponent<ProjectileComponent>(direction, projectileMaxRange);
    projectileComp.Damage = weaponComp.Damage;
    projectileComp.ProjectileRadius = projectileRadius;
    projectileComp.DestructionRadius = weaponComp.DestructionRadius;
    projectileComp.Owner = entity;
    projectileComp.TargetPositionHeightZ1 = SampleHeightAt(scene, mouseWorldPosition, 1);
    projectileComp.DistanceToTargetatFireTime = glm::distance(mouseWorldPosition, playerPosition);
    projectileComp.TargetPositionAtFireTime = mouseWorldPosition;
    projectileComp.ProjectileSped = weaponComp.ProjectileSpeed;
    projectileComp.renderSlot = Engine::ProjectileVisual::GetSlot(ProjectileVisualType::Bullet);


    transformComp.Translation = glm::vec3(playerPosition, 0.0f);
    transformComp.Rotation.z = std::atan2(direction.y, direction.x);

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
