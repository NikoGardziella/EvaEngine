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
    projectileComp.PixelDestructionRadius = weaponComp.DestructionRadius;
    projectileComp.Owner = entity;
    projectileComp.TargetPositionHeightZ1 = SampleHeightAt(scene, mouseWorldPosition, 1);
    projectileComp.DistanceToTargetatFireTime = glm::distance(mouseWorldPosition, playerPosition);
    projectileComp.TargetPositionAtFireTime = mouseWorldPosition;
    EE_INFO("projectileComp.TargetPositionHeightZ1, {}", projectileComp.TargetPositionHeightZ1);
    Engine::SpriteRendererComponent& spriteComp = projectileEntity.AddComponent<Engine::SpriteRendererComponent>();
    spriteComp.Texture = Engine::AssetManager::GetTexture("bullet");
    transformComp.Translation = glm::vec3(playerPosition, 0.0f);
    transformComp.Rotation.z = std::atan2(direction.y, direction.x);

    EE_ASSERT((fabs(glm::length(projectileComp.Direction) - 1.0f) < 1e-3f), "Dir not normalized");
    EE_ASSERT((projectileComp.DistanceToTargetatFireTime > 0.0f), "RayLen must be > 0");
}

float PlayerWeaponSystem::SampleHeightAt(Engine::Scene* scene,
    const glm::vec2& worldXY, int radiusPx /*=0*/)
{
    // 1 G unit = 1 pixel row in world
    const float heightWorldPerUnit = 1.0f / float(TILE_PIXEL_WIDTH);

    // Square grid (matches uploader)
    const int CELL = int(TILE_PIXEL_WIDTH);
    const int CHUNK = int(CHUNK_SIZE) * CELL;

    // World ? atlas pixels (bottom-origin)
    const int px = int(std::floor(worldXY.x * float(CELL)));
    const int py = int(std::floor(worldXY.y * float(CELL)));

    uint8_t maxG = 0;
    bool done = false;

    scene->ForEach<Engine::ChunkRendererComponent>(
        [&](Engine::Entity /*e*/, Engine::ChunkRendererComponent& chunkComp)
        {
            if (done) return;
            if (!chunkComp.PropertiesTexture) return;

            int w = chunkComp.PropertiesTexture->GetWidth();
            int h = chunkComp.PropertiesTexture->GetHeight();
            if (w <= 0) w = CHUNK;
            if (h <= 0) h = CHUNK;

            const glm::ivec2 cc = chunkComp.ChunkCoords;
            const int x0 = cc.x * CHUNK;
            const int y0 = cc.y * CHUNK;
            const int x1 = x0 + w;
            const int y1 = y0 + h;
            if (px < x0 || px >= x1 || py < y0 || py >= y1) return;

            const std::vector<uint8_t>& cpu = chunkComp.PropertiesTexture->GetCPUPixelData();
            const size_t needBytes = size_t(w) * size_t(h) * 4;
            if (cpu.size() < needBytes) { done = true; return; }

            const int lx = px - x0;
            const int ly = py - y0;

            const int r = std::max(0, radiusPx);
            for (int oy = -r; oy <= r; ++oy)
            {
                const int y = ly + oy;
                if ((unsigned)y >= (unsigned)h) continue;
                const size_t row = size_t(y) * size_t(w) * 4;

                for (int ox = -r; ox <= r; ++ox)
                {
                    const int x = lx + ox;
                    if ((unsigned)x >= (unsigned)w) continue;

                    const uint8_t g = cpu[row + size_t(x) * 4 + 1]; // G channel
                    if (g > maxG) maxG = g;
                }
            }
            done = true;
        });

    return float(maxG) * heightWorldPerUnit;
}
