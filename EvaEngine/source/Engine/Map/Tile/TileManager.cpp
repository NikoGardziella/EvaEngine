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

        glm::vec2 initialFocusPos = { 0, 0 };
        const float registerRadius = 80.0f;
        const float registerRadiusSq = registerRadius * registerRadius;

        VulkanRenderer2D::GetBindlessDescriptorSetRenderer()->EvictAllTiles();

        StreamingConfig config;
        config.gpuRadius = 30.0f;
        config.cpuRadius = 60.0f;
        config.hysteresis = 2.0f;
        config.maxTransitionsPerFrame = 64;
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

                    if (glm::distance2(center, initialFocusPos) > registerRadiusSq)
                        continue;

                    const std::vector<uint8_t>* color = nullptr;
                    const std::vector<uint8_t>* props = nullptr;

                    if (!GetOriginalTileData(t.UID, color, props))
                    {
                        EE_CORE_WARN("BuildInitialResidency: missing original data for uid {:016x} '{}'",
                            t.UID, t.name);
                        continue;
                    }

                    m_centerByUID[t.UID] = center;

                    float dist = glm::distance(center, initialFocusPos);
                    TileResidency initialResidency =
                        (dist <= config.cpuRadius) ? TileResidency::CPU : TileResidency::Disk;

                    m_streaming.RegisterTileInitial(
                        t.UID,
                        center,
                        *color,
                        *props,
                        t.opaqueMin,
                        t.opaqueMax,
                        t.name,
                        UINT32_MAX,
                        initialResidency
                    );
                }
            });

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
        ProjectileVisual::RegisterVisual(ProjectileVisualType::Bullet, bulletUID, bulletSlot);

        uint64_t grenadeUID = HashUtils::MakeTileUID_String("grenade");
        uint32_t grenadeSlot = EnsureVisualResident(grenadeUID);
        ProjectileVisual::RegisterVisual(ProjectileVisualType::Grenade, grenadeUID, grenadeSlot);
    }

    uint32_t TileManager::EnsureVisualResident(uint64_t uid)
    {
        auto slotIt = m_slotByUID.find(uid);
        if (slotIt != m_slotByUID.end() && slotIt->second != UINT32_MAX)
            return slotIt->second;

        const std::vector<uint8_t>* color = nullptr;
        const std::vector<uint8_t>* props = nullptr;

        if (!GetOriginalTileData(uid, color, props))
        {
            EE_CORE_ERROR("EnsureVisualResident: missing source data for uid {:016x}", uid);
            return UINT32_MAX;
        }

        if (!color || !props || color->empty() || props->empty())
        {
            EE_CORE_ERROR("EnsureVisualResident: empty source data for uid {:016x}", uid);
            return UINT32_MAX;
        }

        VulkanContext* ctx = VulkanContext::Get();
        VkCommandBuffer cb = ctx->BeginSingleTimeCommands();

        uint32_t slot = VulkanRenderer2D::GetBindlessDescriptorSetRenderer()->EnsureTileResidentFromRaw(
            uid,
            color->data(), color->size(),
            props->data(), props->size(),
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
                        continue;

                    uint64_t uid = tile.UID;
                    if (!uid)
                    {
                        uid = HashUtils::MakeTileUID(
                            (uint64_t)idComp.ID,
                            tile.position,
                            float(TILE_SIZE),
                            (uint32_t)tile.Category);

                        tile.UID = uid;
                    }

                    TileTypeKey key = MakeTileTypeKey(tile);
                    m_typeByUID[uid] = key;

                    // Extract shared data once per type
                    if (!m_colorByType.count(key) || !m_propsByType.count(key))
                    {
                        glm::ivec2 outOpaqueMin = glm::ivec2(TILE_PIXEL_WIDTH, TILE_PIXEL_HEIGHT);
                        glm::ivec2 outOpaqueMax = glm::ivec2(-1);

                        std::vector<uint8_t> colorRGBA, propsRGBA;
                        int w = 0, h = 0;

                        if (!AssetManager::ExtractPixelsAndPropertiesFromTilePallette(
                            tile, colorRGBA, propsRGBA, w, h, outOpaqueMin, outOpaqueMax))
                        {
                            EE_CORE_WARN("ExtractPixelsFromTilePallette failed for tile '{}'", tile.name);
                            continue;
                        }

                        m_colorByType.emplace(key, ColorTemplate{ w, h, std::move(colorRGBA) });
                        m_propsByType.emplace(key, PropsTemplate{ w, h, std::move(propsRGBA) });
                        m_opaqueByType.emplace(key, std::make_pair(outOpaqueMin, outOpaqueMax));
                    }

                    // Apply shared opaque bounds to every instance
                    auto opIt = m_opaqueByType.find(key);
                    if (opIt != m_opaqueByType.end())
                    {
                        tile.opaqueMin = opIt->second.first;
                        tile.opaqueMax = opIt->second.second;
                    }

                    // Fill per-UID maps for every tile, not just first of type
                    const auto& sharedColor = m_colorByType.at(key);
                    const auto& sharedProps = m_propsByType.at(key);

                   
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

                TileTypeKey bulletKey{};
                bulletKey.name = "bullet_sprite";
                bulletKey.uv = glm::vec4(0.0f);
                bulletKey.category = eTileCategory::Undefined; 
                bulletKey.direction = eTileDirection::Center; 

                m_typeByUID[bulletUID] = bulletKey;
                m_colorByType[bulletKey] = ColorTemplate{ w, h, std::move(colorRGBA) };
                m_propsByType[bulletKey] = PropsTemplate{ w, h, std::move(propsRGBA) };
                m_opaqueByType[bulletKey] = {
                    glm::ivec2(0, 0),
                    glm::ivec2(w, h)
                };

               
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
                TileTypeKey grenadeKey{};
                grenadeKey.name = "grenade";
                grenadeKey.uv = glm::vec4(0.0f);
                grenadeKey.category = eTileCategory::Undefined;
                grenadeKey.direction = eTileDirection::Center;

                m_typeByUID[grenadeUID] = grenadeKey;
                m_colorByType[grenadeKey] = ColorTemplate{ w, h, std::move(colorRGBA) };
                m_propsByType[grenadeKey] = PropsTemplate{ w, h, std::move(propsRGBA) };
                m_opaqueByType[grenadeKey] = {
                    glm::ivec2(0, 0),
                    glm::ivec2(w, h)
                };
               
            }
        }
    }


    TileTypeKey TileManager::MakeTileTypeKey(const TileInfo& tile) const
    {
        TileTypeKey key{};
        key.name = tile.name;
        key.uv = tile.UV;
        key.category = tile.Category;
        key.direction = tile.TileDirection;
        return key;
    }


    bool TileManager::GetOriginalTileData(uint64_t uid, const std::vector<uint8_t>*& color, const std::vector<uint8_t>*& props) const
    {
        color = nullptr;
        props = nullptr;

        auto itType = m_typeByUID.find(uid);
        if (itType == m_typeByUID.end())
            return false;

        const TileTypeKey& key = itType->second;

        auto itColor = m_colorByType.find(key);
        auto itProps = m_propsByType.find(key);

        if (itColor == m_colorByType.end() || itProps == m_propsByType.end())
            return false;

        color = &itColor->second.rgba;
        props = &itProps->second.rgba;
        return true;
    }

    void TileManager::ClearTemplates()
    {
  

        m_colorByType.clear();
        m_propsByType.clear();
        m_typeByUID.clear();
        m_opaqueByType.clear();
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