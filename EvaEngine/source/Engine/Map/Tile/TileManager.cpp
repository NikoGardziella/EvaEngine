#include "pch.h"
#include "TileManager.h"
#include "Engine/AssetManager/AssetManager.h"
#include <Engine/Math/HashUtils.h>
#include <Engine/Scene/Component.h>
#include <Engine/Scene/Components/Render/TileComponent.h>
#include <Engine/Platform/Vulkan/VulkanContext.h>
#include <Engine/Renderer/Renderer2D/VulkanRenderer2D.h>
#include <Engine/Map/Projectile/ProjectileVisualRegistry.h>
#include <Engine/Map/Grid/TileCollisionMask.h>

namespace Engine {

   
    // Build / UID map -> open one - shot CB -> upload color / props to array
    // layers & write descriptors -> cache per - slot origin / content rect -> submit & wait.
    //After this, per frame  only submit instances;
    // does not scale for very large maps. need to rebuild this 
    
    void TileManager::BuildInitialResidency(Scene* scene)
    {
        VulkanRenderer2D::GetBindlessDescriptorSetRenderer()->EvictAllTiles();
        EE_PROFILE_FUNCTION();

        // Initialize streaming system
        StreamingConfig config;
        config.gpuRadius = 30.0f;
        config.cpuRadius = 60.0f;
        config.hysteresis = 2.0f;
        config.maxTransitionsPerFrame = 4;
        config.cachePath = "tile_cache";
        m_streaming.InitTileStreaming(this, config);

        {
            EE_PROFILE_SCOPE("get tile center");
            m_centerByUID.clear();
            scene->ForEachConst<TransformComponent, TileComponent, IDComponent>(
                [&](Entity e, const TransformComponent& tr, const TileComponent& tc, const IDComponent& id) {
                    for (size_t i = 0; i < tc.tiles.size(); ++i)
                    {
                        const TileInfo& t = tc.tiles[i];
                        if (t.Category == eTileCategory::Terrain) continue;
                        glm::vec2 center = glm::vec2(tr.Translation) + t.position;
                        m_centerByUID[t.UID] = center;
                    }
                });
        }

        VulkanContext* ctx = VulkanContext::Get();
        VkCommandBuffer cb = ctx->BeginSingleTimeCommands();

        for (const auto& [uid, col] : m_colorByUID)
        {
            auto pit = m_propsByUID.find(uid);
            if (pit == m_propsByUID.end())
            {
                EE_CORE_WARN("No props template for uid {:016x}", uid);
                continue;
            }
            const PropsTemplate& pr = pit->second;

            uint32_t slot = VulkanRenderer2D::GetBindlessDescriptorSetRenderer()->EnsureTileResidentFromRaw(uid,
                col.rgba.data(), col.rgba.size(), pr.rgba.data(), pr.rgba.size(), cb);
            m_slotByUID[uid] = slot;

            // *** NEW: Register with streaming system ***
            glm::vec2 center = { 0.0f, 0.0f };
            auto cit = m_centerByUID.find(uid);
            if (cit != m_centerByUID.end()) center = cit->second;

            // Get opaque bounds if available, otherwise default to full tile
            glm::ivec2 opaqueMin = { 0, 0 };
            glm::ivec2 opaqueMax = { TILE_PIXEL_WIDTH, TILE_PIXEL_HEIGHT };
            // opaqueMin = tileInfo.opaqueMin;
            // opaqueMax = tileInfo.opaqueMax;

            m_streaming.RegisterTile(uid, center,
                col.rgba, pr.rgba,
                opaqueMin, opaqueMax,
                "", // tileName — fill in if you have it
                slot);
        }

        ctx->EndSingleTimeCommands(cb);

        scene->ForEach<TileComponent>([&](Entity e, TileComponent& tc)
            {
                for (auto& t : tc.tiles)
                {
                    auto it = m_slotByUID.find(t.UID);
                    if (it != m_slotByUID.end()) t.Slot = it->second;
                }
            });
        
        uint64_t bulletUID = HashUtils::MakeTileUID_String("bullet_sprite");
        uint32_t bulletSlot = GetSlotForUID(bulletUID);
        ProjectileVisual::RegisterVisual(ProjectileVisualType::Bullet, bulletUID, bulletSlot);

        uint64_t grenadeUID = HashUtils::MakeTileUID_String("grenade");
        uint32_t grenadeSlot = GetSlotForUID(grenadeUID);
        ProjectileVisual::RegisterVisual(ProjectileVisualType::Grenade, grenadeUID, grenadeSlot);
        
    }



    void TileManager::BuildTemplatesForScene(Scene* scene)
    {
        ClearTemplates();

        scene->ForEach<TransformComponent, TileComponent, IDComponent>(
            [&](Entity e, TransformComponent& tr, TileComponent& tc, IDComponent& idComp)
            {
                for (TileInfo& tile : tc.tiles)
                {
                    if (tile.Category == eTileCategory::Terrain)
                    {
                        continue;
                    }
       
                    // If not set, compute it here the same when placing tiles.
                    uint64_t uid = tile.UID;
                    if (!uid)
                    {
                        // deltaGround == t.position in layout
                        uid = HashUtils::MakeTileUID((uint64_t)idComp.ID, tile.position, float(TILE_SIZE));
                    }
                    EE_CORE_INFO("tile build template {}", uid);

                    // Skip if already cached
                    if (m_colorByUID.count(uid) && m_propsByUID.count(uid))
                        continue;

                    glm::ivec2 outOpaqueMin = glm::ivec2(TILE_PIXEL_WIDTH, TILE_PIXEL_HEIGHT);
                    glm::ivec2 outOpaqueMax = glm::ivec2(-1);

                    std::vector<uint8_t> colorRGBA, propsRGBA;
                    int w = 0, h = 0;
                    if (!AssetManager::ExtractPixelsAndPropertiesFromTilePallette(tile, colorRGBA, propsRGBA, w, h, outOpaqueMin, outOpaqueMax))
                    {
                        EE_CORE_WARN("ExtractPixelsFromTilePallette failed for tile '{}'", tile.name);
                        continue;
                    }
                    tile.opaqueMax = outOpaqueMax;
                    tile.opaqueMin = outOpaqueMin;


                    m_colorByUID.emplace(uid, ColorTemplate{ w, h, std::move(colorRGBA) });
                    m_propsByUID.emplace(uid, PropsTemplate{ w, h, std::move(propsRGBA) });
                }
            });
        
        {
            // projectiles
            std::vector<uint8_t> colorRGBA;
            std::vector<uint8_t> healthData;
            int w = 0, h = 0;

           
            if (!AssetManager::GetTexturePixelData("Fire_small_asset",
                colorRGBA, healthData, w, h))
            {
                EE_CORE_WARN("GetTexturePixelData failed for bullet sprite");
            }
            else
            {
                const size_t pixelCount = size_t(w) * size_t(h);

                // Convert healthData (1 byte per pixel) -> RGBA props
                std::vector<uint8_t> propsRGBA(pixelCount * 4u, 0u);
                for (size_t i = 0; i < pixelCount; ++i)
                {
                    uint8_t health = healthData[i];
                    propsRGBA[i * 4 + 0] = health; // R = health
                    propsRGBA[i * 4 + 1] = 0;
                    propsRGBA[i * 4 + 2] = 0;
                    propsRGBA[i * 4 + 3] = 0;      // or flags if you want
                }

                // Make a unique UID for the bullet
                uint64_t bulletUID = HashUtils::MakeTileUID_String("bullet_sprite");

                m_colorByUID.emplace(bulletUID, ColorTemplate{ w, h, std::move(colorRGBA) });
                m_propsByUID.emplace(bulletUID, PropsTemplate{ w, h, std::move(propsRGBA) });
            }
        }
        {
            // grenade
            std::vector<uint8_t> colorRGBA;
            std::vector<uint8_t> healthData;
            int w = 0, h = 0;


            if (!AssetManager::GetTexturePixelData("grenade",
                colorRGBA, healthData, w, h))
            {
                EE_CORE_WARN("GetTexturePixelData failed for grenade sprite");
            }
            else
            {
                const size_t pixelCount = size_t(w) * size_t(h);

                // Convert healthData (1 byte per pixel) -> RGBA props
                std::vector<uint8_t> propsRGBA(pixelCount * 4u, 0u);
                for (size_t i = 0; i < pixelCount; ++i)
                {
                    uint8_t health = healthData[i];
                    propsRGBA[i * 4 + 0] = health; // R = health
                    propsRGBA[i * 4 + 1] = 0;
                    propsRGBA[i * 4 + 2] = 0;
                    propsRGBA[i * 4 + 3] = 0;      // or flags if you want
                }

                // Make a unique UID for the bullet
                uint64_t grenadeUID = HashUtils::MakeTileUID_String("grenade");

                m_colorByUID.emplace(grenadeUID, ColorTemplate{ w, h, std::move(colorRGBA) });
                m_propsByUID.emplace(grenadeUID, PropsTemplate{ w, h, std::move(propsRGBA) });
            }
        }
    }


    void TileManager::ClearTemplates()
    {
        m_colorByUID.clear();
        m_propsByUID.clear();

        
        m_colorByUID.rehash(0);
        m_propsByUID.rehash(0);
    }

    void TileManager::Update(Scene* scene, glm::vec2 playerPos)
    {
        m_streaming.SyncDirtyFlags(TileBlockedMaskCPU::DirtyTileRuntime);

        m_streaming.UpdateStreaming(scene, playerPos);
    }


    void TileManager::Shutdown()
    {
         m_streaming.Shutdown();
      }
}