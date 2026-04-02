#include "pch.h"
#include "SceneRoofTileAccess.h"

#include <Engine/Scene/Scene.h>
#include <Engine/Scene/Entity.h>
#include <Engine/Scene/Components/Render/TileComponent.h>
#include <Engine/Renderer/Renderer2D/VulkanRenderer2D.h>
#include <Engine/Scene/Components/Physics/PhysicsComponent.h>
#include "Engine/Scene/Components/Physics/PhysicUtils.h"

namespace Engine {
    static inline int Quantize(float v, float step)
    {
        // round to nearest step index
        return (int)std::floor(v / step + 0.5f);
    }
    static inline float Dequantize(int i, float step)
    {
        return (float)i * step;
    }

    bool SceneRoofTileAccess::HasRoof(const glm::ivec2& p) const
    {
        return FindFirstTileAt(p, true, false, nullptr, nullptr);
    }

    bool SceneRoofTileAccess::HasSupport(const glm::ivec2& p) const
    {
        return FindFirstTileAt(p, false, true, nullptr, nullptr);
    }

    void SceneRoofTileAccess::RemoveRoof(const glm::ivec2& p)
    {
        Entity owner{};
        size_t tileIndex = SIZE_MAX;

        if (!FindFirstTileAt(p, true, false, &owner, &tileIndex))
            return;

        if (!owner || tileIndex == SIZE_MAX)
            return;

        auto& tc = owner.GetComponent<TileComponent>();
        if (tileIndex >= tc.tiles.size())
            return;

        const float pxW = float(TILE_SIZE) / float(TILE_PIXEL_WIDTH);
        const float gravityMag = 9.81f;

        float simulateSeconds = 0.4f;

        glm::vec2 v0 = glm::vec2(+20.0f * pxW, +40.0f * pxW); // tweak
        float     w0 = glm::radians(120.0f);
        bool destroyOnFinish = true;


        PhysicsUtils::DetachTileAsPhysicsEntity(m_scene, owner, tileIndex, glm::vec3(0.f, 0.5f, 0.f));

    }

  

   

    bool SceneRoofTileAccess::FindFirstTileAt(const glm::ivec2& p, bool wantRoof, bool wantSupport,
        Entity* outOwner, size_t* outIndex) const
    {
        if (!m_scene) return false;


        bool found = false;
        Entity owner{};
        size_t idx = SIZE_MAX;

        m_scene->ForEachConst<TileComponent, IDComponent, TransformComponent>(
            [&](Entity e, const TileComponent& tc, const IDComponent&, const TransformComponent& tileTransformComp)
            {
                for (size_t i = 0; i < tc.tiles.size(); ++i)
                {
                    const auto& tile = tc.tiles[i];

                    glm::vec2 tilePos =
                        glm::vec2(tileTransformComp.Translation.x, tileTransformComp.Translation.y)
                        + tile.position;

                    // Convert tile world position to integer cell coords
                    glm::ivec2 tileCell(
                        Quantize(tilePos.x, ROOF_STEP_X),
                        Quantize(tilePos.y, ROOF_STEP_Y)
                    );
                    
                    
                
                    // Now compare in cell space (integer compare, stable)
                    if (tileCell != p)
                        continue;
                   
                    if (wantRoof)
                    {
                        const bool isRoofTile = tile.Category == eTileCategory::Roofs ||
                                                (tile.IsRoof && tile.Category == eTileCategory::dynamicObjects);

                        if (!isRoofTile)
                        {
                            continue;
                        }
                    }

                    if (wantSupport)
                    {
                        if (tile.Category != eTileCategory::Buildings)
                        {
                            continue;
                        }
                        if (!tile.IsSupportingRoof)
                        {
                            continue;
                        }


                    }
                    
                    EE_CORE_INFO("tile found at {}", tileCell);

                    owner = e;
                    idx = i;
                    found = true;
                    return;
                }
            });

        if (found)
        {
            if (outOwner) *outOwner = owner;
            if (outIndex) *outIndex = idx;
        }
        return found;
    }

} 