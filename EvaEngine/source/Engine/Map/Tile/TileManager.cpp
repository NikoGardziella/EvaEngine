#include "pch.h"
#include "TileManager.h"
#include "Engine/AssetManager/AssetManager.h"
#include <Engine/Math/HashUtils.h>
#include <Engine/Scene/Component.h>
#include <Engine/Scene/Components/Render/TileComponent.h>
#include <Engine/Platform/Vulkan/VulkanContext.h>
#include "Engine/Renderer/VulkanRenderer2D.h"

namespace Engine {

   

    void TileManager::StreamInitialResidency(Scene* scene)
    {

        m_centerByUID.clear();
        scene->ForEachConst<TransformComponent, TileComponent, IDComponent>(
            [&](Entity e, const TransformComponent& tr, const TileComponent& tc, const IDComponent& id) {
                for (size_t i = 0; i < tc.tiles.size(); ++i)
                {
                    const TileInfo& t = tc.tiles[i];
                    if (t.Category == eTileCategory::Terrain) continue;

                    glm::vec2 center = glm::vec2(tr.Translation) + t.position;
                    uint64_t uid = HashUtils::MakeTileUID((uint64_t)id.ID, t.position, float(TILE_SIZE), 0, t.NameHash);
                    m_centerByUID[uid] = center;
                }
            });

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

            std::unordered_map<uint64_t, glm::vec2>::const_iterator cit = m_centerByUID.find(uid);
            glm::vec2 center = cit->second;
            glm::vec2 origin = BottomLeftFromCenter(center, m_tileWorldW, m_tileWorldH);


            ContentRect rc = ComputeOpaqueBounds(col.rgba, TILE_PIXEL_HEIGHT, TILE_PIXEL_WIDTH);
            VulkanRenderer2D::SetSlotContentRect(slot, { rc.x, rc.y }, { rc.w, rc.h });
        }

        ctx->EndSingleTimeCommands(cb);
    }

    void TileManager::BuildTemplatesForScene(Scene* scene)
    {
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
                    uint64_t uid = t.NameHash;
                    if (!uid)
                    {
                        // deltaGround == t.position in your layout
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
    }

}