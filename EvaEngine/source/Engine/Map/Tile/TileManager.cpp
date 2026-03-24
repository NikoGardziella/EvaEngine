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

        EE_PROFILE_FUNCTION();
        glm::vec2 initialFocusPos = { 0,0 };
        VulkanRenderer2D::GetBindlessDescriptorSetRenderer()->EvictAllTiles();

        StreamingConfig config;
        config.gpuRadius = 30.0f;
        config.cpuRadius = 60.0f;
        config.hysteresis = 2.0f;
        config.maxTransitionsPerFrame = 64; // startup can be bigger than runtime
        config.cachePath = "tile_cache";
        m_streaming.InitTileStreaming(this, config);

        m_centerByUID.clear();
        m_slotByUID.clear();

        scene->ForEachConst<TransformComponent, TileComponent, IDComponent>(
            [&](Entity e, const TransformComponent& tr, const TileComponent& tc, const IDComponent& id)
            {
                for (const TileInfo& t : tc.tiles)
                {
                    if (t.Category == eTileCategory::Terrain)
                        continue;

                    glm::vec2 center = glm::vec2(tr.Translation) + glm::vec2(t.position);
                    m_centerByUID[t.UID] = center;
                }
            });

        // Register everything WITHOUT uploading
        for (const auto& [uid, col] : m_colorByUID)
        {
            auto pit = m_propsByUID.find(uid);
            if (pit == m_propsByUID.end())
            {
                EE_CORE_WARN("No props template for uid {:016x}", uid);
                continue;
            }

            const PropsTemplate& pr = pit->second;

            glm::vec2 center = { 0.0f, 0.0f };
            auto cit = m_centerByUID.find(uid);
            if (cit != m_centerByUID.end())
                center = cit->second;

            glm::ivec2 opaqueMin = { 0, 0 };
            glm::ivec2 opaqueMax = { TILE_PIXEL_WIDTH, TILE_PIXEL_HEIGHT };

            float dist = glm::distance(center, initialFocusPos);

            uint32_t slot = UINT32_MAX;

            // Near tiles start in CPU and will be uploaded immediately below.
            // Far tiles can start directly on Disk if clean.
            TileResidency initialResidency = (dist <= config.cpuRadius)
                ? TileResidency::CPU
                : TileResidency::Disk;

            m_streaming.RegisterTileInitial(uid, center,
                col.rgba, pr.rgba,
                opaqueMin, opaqueMax,
                "",
                slot,
                initialResidency);

            
        }

        // Upload only tiles near startup focus/player/camera
        m_streaming.PrimeInitialGPUResidency(scene, initialFocusPos);

        scene->ForEach<TileComponent>([&](Entity e, TileComponent& tc)
            {
                for (auto& t : tc.tiles)
                {
                    t.Slot = m_streaming.GetSlot(t.UID);
                }
            });

        uint64_t bulletUID = HashUtils::MakeTileUID_String("bullet_sprite");
        uint32_t bulletSlot = EnsureVisualResident(bulletUID);
        if (bulletSlot == UINT32_MAX)
        {
            EE_CORE_ERROR("invalid bulletSlot");
            
        }
        
        ProjectileVisual::RegisterVisual(ProjectileVisualType::Bullet, bulletUID, bulletSlot);

        uint64_t grenadeUID = HashUtils::MakeTileUID_String("grenade");
        uint32_t grenadeSlot = EnsureVisualResident(grenadeUID);
        if (grenadeSlot == UINT32_MAX)
        {
            EE_CORE_ERROR("invalid bulletSlot");
        }

        ProjectileVisual::RegisterVisual(ProjectileVisualType::Grenade, grenadeUID, grenadeSlot);

    }

    uint32_t TileManager::EnsureVisualResident(uint64_t uid)
    {
        auto slotIt = m_slotByUID.find(uid);
        if (slotIt != m_slotByUID.end() && slotIt->second != UINT32_MAX)
            return slotIt->second;

        auto colIt = m_colorByUID.find(uid);
        if (colIt == m_colorByUID.end())
        {
            EE_CORE_ERROR("EnsureVisualResident: missing color data for uid {:016x}", uid);
            return UINT32_MAX;
        }

        auto propIt = m_propsByUID.find(uid);
        if (propIt == m_propsByUID.end())
        {
            EE_CORE_ERROR("EnsureVisualResident: missing props data for uid {:016x}", uid);
            return UINT32_MAX;
        }

        VulkanContext* ctx = VulkanContext::Get();
        VkCommandBuffer cb = ctx->BeginSingleTimeCommands();

        uint32_t slot = VulkanRenderer2D::GetBindlessDescriptorSetRenderer()->EnsureTileResidentFromRaw(
            uid,
            colIt->second.rgba.data(), colIt->second.rgba.size(),
            propIt->second.rgba.data(), propIt->second.rgba.size(),
            cb);

        ctx->EndSingleTimeCommands(cb);

        m_slotByUID[uid] = slot;
        return slot;
    }

    void TileManager::BuildTemplatesForScene(Scene* scene)
    {
        EE_PROFILE_FUNCTION();

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
                        uid = HashUtils::MakeTileUID((uint64_t)idComp.ID, tile.position, float(TILE_SIZE), (uint32_t)tile.Category);
                    }
                   // EE_CORE_INFO("tile build template {}", uid);

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