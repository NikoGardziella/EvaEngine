#include "pch.h"
#include "TileManager.h"
#include "Engine/AssetManager/AssetManager.h"
#include <Engine/Math/HashUtils.h>
#include <Engine/Scene/Component.h>
#include <Engine/Scene/Components/Render/TileComponent.h>
#include <Engine/Platform/Vulkan/VulkanContext.h>
#include "Engine/Renderer/VulkanRenderer2D.h"
#include <Engine/Map/Projectile/ProjectileVisualRegistry.h>

namespace Engine {

   
    // Build / UID map -> open one - shot CB -> upload color / props to array
    // layers & write descriptors -> cache per - slot origin / content rect -> submit & wait.
    //After this, per frame  only submit instances;
    // does not scale for very large maps. need to rebuild this 
    
    void TileManager::BuildInitialResidency(Scene* scene)
    {

        VulkanRenderer2D::GetBindlessDescriptorSetRenderer()->EvictAllTiles();
        EE_PROFILE_FUNCTION();

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
                        m_centerByUID[t.UID] = center; // cache thios

                    }
                });

        }

        VulkanContext* ctx = VulkanContext::Get();
        VkCommandBuffer cb = ctx->BeginSingleTimeCommands();

     

        // For each UID we’ve prepared, upload once and write descriptors.
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

        scene->ForEachConst<TransformComponent, TileComponent, IDComponent>(
            [&](Entity e, const TransformComponent& tr, const TileComponent& tc, const IDComponent& idComp)
            {
                for (const TileInfo& t : tc.tiles)
                {
                    if (t.Category == eTileCategory::Terrain)
                    {
                        continue;
                    }
       
                    // If not set, compute it here the same when placing tiles.
                    uint64_t uid = t.UID;
                    if (!uid)
                    {
                        // deltaGround == t.position in layout
                        uid = HashUtils::MakeTileUID((uint64_t)idComp.ID, t.position, float(TILE_SIZE));
                    }

                    // Skip if already cached
                    if (m_colorByUID.count(uid) && m_propsByUID.count(uid))
                        continue;


                    std::vector<uint8_t> colorRGBA, propsRGBA;
                    int w = 0, h = 0;
                    if (!AssetManager::ExtractPixelsFromTilePallette(t, colorRGBA, propsRGBA, w, h))
                    {
                        EE_CORE_WARN("ExtractPixelsFromTilePallette failed for tile '{}'", t.name);
                        continue;
                    }


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



}