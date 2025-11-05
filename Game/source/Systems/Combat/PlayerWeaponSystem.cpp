#include "PlayerWeaponSystem.h"
#include <Engine/Scene/Components/Combat/WeaponComponent.h>
#include <Engine/AssetManager/AssetManager.h>
#include <Engine/Debug/Instrumentor.h>
#include <Engine/Scene/Components/Vehicles/DriverComponent.h>
#include <Engine/Scene/Components/Projectiles/ProjectileComponent.h>
#include <Engine/Scene/Scene.h>
#include <Engine/Scene/Components/Render/ChunkRendererComponent.h>
#include <Engine/Core/Log.h>


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

    // Players with weapons
    scene->ForEach<Engine::TransformComponent, WeaponComponent>(
        [&](Engine::Entity playerEntity, Engine::TransformComponent& playerTransformComp, WeaponComponent& weaponComp)
        {
            // cooldown tick
            if (weaponComp.Cooldown > 0.0f)
                weaponComp.Cooldown -= deltaTime;

            if (!hasPrimaryCamera) return;

            // fire?
            if (Engine::Input::IsMouseButtonPressed(Engine::Mouse::Button0) &&
                weaponComp.Cooldown <= 0.0f)
            {
                

               
                    ShootProjectile(playerEntity, playerTransformComp.Translation,
                        mouseWorldPosition, scene, weaponComp);

                    weaponComp.Cooldown = weaponComp.FireRate;
                
            }
        });
}


void PlayerWeaponSystem::ShootProjectile(Engine::Entity entity,
    const glm::vec2& playerPosition, const glm::vec2& mouseWorldPosition, Engine::Scene* scene, const WeaponComponent& weaponComp)
{
    EE_PROFILE_FUNCTION();

    // disable shooting from a car now
    if (entity.HasComponent<DriverComponent>())
        return;

    glm::vec2 direction = glm::normalize(mouseWorldPosition - playerPosition);
    Engine::Entity& projectileEntity = scene->CreateEntity("Projectile");

    Engine::TransformComponent& transformComp = projectileEntity.AddComponent<Engine::TransformComponent>();
	float projectileMaxRange = 3.0f; // fix this
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
   // EE_INFO("projectileComp.TargetPositionHeightZ1, {}", projectileComp.TargetPositionHeightZ1);
    Engine::SpriteRendererComponent& spriteComp = projectileEntity.AddComponent<Engine::SpriteRendererComponent>();
    spriteComp.Texture = Engine::AssetManager::GetTexture("bullet");
    transformComp.Translation = glm::vec3(playerPosition, 0.0f);
    transformComp.Rotation.z = std::atan2(direction.y, direction.x);

    EE_ASSERT((fabs(glm::length(projectileComp.Direction) - 1.0f) < 1e-3f), "Dir not normalized");
    EE_ASSERT((projectileComp.DistanceToTargetatFireTime > 0.0f), "RayLen must be > 0");
}

float SampleHeightAt_FromTileCenterFudged(
    const glm::vec2& worldXY,
    const glm::vec2& tileCenter,
    const glm::vec2& tileSizeWorld,
    float pxWorld,
    const glm::vec2& originBiasWorld = glm::vec2(0.5f, 0.0f), // <- your current bias
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
