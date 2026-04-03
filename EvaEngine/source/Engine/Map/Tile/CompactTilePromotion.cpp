#include "pch.h"
#include "CompactTilePromotion.h"

#include "Engine/Scene/Scene.h"
#include "Engine/Scene/Entity.h"
#include "Engine/Scene/Component.h"
#include "Engine/Scene/Components/Render/TileComponent.h"

#include "Engine/Map/Utils/IsoTileUtils.h"
#include "Engine/Math/HashUtils.h"
#include "Engine/AssetManager/AssetManager.h"

namespace Engine
{
    TileInfo CompactTilePromotion::BuildRuntimeTileFromDefinition(const TileDefinition& def, const glm::vec2& localPos)
    {
        TileInfo t{};
        t.position = localPos;
        t.UV = def.UV;
        t.name = def.Name;
        t.IsDestructible = def.IsDestructible;
        t.IsSupportingRoof = def.IsSupportingRoof;
        t.IsRoof = def.IsRoof;
        t.Category = def.Category;
        t.Material = def.Material;
        t.TileHealth = def.BaseHealth;
        t.TileDirection = def.Direction;
        t.UID = 0;
        t.Slot = UINT32_MAX;
        t.opaqueMin = glm::ivec2(0);
        t.opaqueMax = glm::ivec2(0);
        return t;
    }



    Entity CompactTilePromotion::PromoteGroup(Scene* scene, uint64_t groupId)
    {
        Entity e{};

        if (!scene || groupId == 0)
        {
            EE_CORE_WARN("PromoteGroup failed: invalid scene or groupId");
            return e;
        }

        CompactTileMap& compactMap = scene->GetCompactTileMap();
        TileDefinitionRegistry& defs = scene->GetTileDefinitions();

        const std::vector<glm::ivec2>* cells = compactMap.GetCellsForGroup(groupId);
        if (!cells || cells->empty())
        {
            EE_CORE_WARN("PromoteGroup failed: no cells for group {}", groupId);
            return e;
        }

        struct PendingTile
        {
            glm::ivec2 worldCell{};
            uint16_t typeId = 0;
            const TileDefinition* def = nullptr;
        };

        std::vector<PendingTile> tilesToPromote;
        tilesToPromote.reserve(cells->size());

        // Gather only the compact tiles that belong to this group
        for (const glm::ivec2& worldCell : *cells)
        {
            const std::vector<CompactTile>* compactTiles = compactMap.GetTiles(worldCell);
            if (!compactTiles)
                continue;

            for (const CompactTile& compact : *compactTiles)
            {
                if (compact.IsEmpty())
                    continue;

                if (compact.Flags & CompactTileFlags::Promoted)
                    continue;

                if (compact.GroupId != groupId)
                    continue;

                const TileDefinition* def = defs.Get(compact.TypeId);
                if (!def)
                {
                    EE_CORE_WARN("PromoteGroup: missing TileDefinition for typeId {}", compact.TypeId);
                    continue;
                }

                PendingTile p{};
                p.worldCell = worldCell;
                p.typeId = compact.TypeId;
                p.def = def;
                tilesToPromote.push_back(p);
            }
        }

        if (tilesToPromote.empty())
        {
            return e;
        }

        // Create one runtime entity for the whole group
        
        scene->ForEach<IDComponent>(
            [&](Engine::Entity entity, IDComponent& entityID)
            {
                if(groupId == static_cast<uint64_t>(entityID.ID))
                {
                    e = entity;

                }
            });

        if (!e)
        {
            e = scene->CreateEntity("PromotedGroup");
        }




        TransformComponent* tr = nullptr;
        if (e.HasComponent<TransformComponent>())
        {
            tr = e.TryGetComponent<TransformComponent>();
        }
        else
        {
            tr = &e.AddComponent<TransformComponent>();
            const glm::vec2 rootWorldPos = IsoTileUtils::IsoToWorldGround(tilesToPromote[0].worldCell);

            tr->Translation = glm::vec3(rootWorldPos, 0.0f);
        }

        TileComponent& tc = e.GetOrAddComponent<TileComponent>();
        IDComponent& id = e.GetOrAddComponent<IDComponent>();

        // Use first tile as root
        ///

        // Build runtime tiles relative to root
        tc.tiles.reserve(tilesToPromote.size());




        for (const PendingTile& p : tilesToPromote)
        {
            const glm::vec2 tileWorldPos = IsoTileUtils::IsoToWorldGround(p.worldCell);
            const glm::vec2 localPos = tileWorldPos - glm::vec2(tr->Translation);

            TileInfo runtimeTile = BuildRuntimeTileFromDefinition(*p.def, localPos);

            runtimeTile.UID = HashUtils::MakeTileUID(
                (uint64_t)id.ID,
                runtimeTile.position,
                float(TILE_SIZE),
                (uint32_t)runtimeTile.Category,
                p.def->Direction);

            EE_CORE_INFO("runtimeTile.UID {}", runtimeTile.UID);
            tc.tiles.push_back(runtimeTile);
        }

        // Mark only this group's cells as hidden/promoted
        for (PendingTile& p : tilesToPromote)
        {
            CompactTile* compact = compactMap.FindTile(p.worldCell, p.typeId);
            if (!compact)
                continue;

            compact->Flags |= CompactTileFlags::Promoted;
            compact->Flags |= CompactTileFlags::Hidden;
            compactMap.MarkChunkDirtyForCell(p.worldCell);
        }

        

        return e;
    }

    uint16_t CompactTilePromotion::GetOrCreateDefinitionForRuntimeTile(Scene* scene, const TileInfo& tile)
    {
        if (!scene)
            return 0;

        TileDefinitionRegistry& defs = scene->GetTileDefinitions();

        TileTypeKey key{};
        key.name = tile.name;
        key.uv = tile.UV;
        key.category = tile.Category;
        key.direction = tile.TileDirection;

        uint16_t existingTypeId = 0;
        if (defs.FindTypeId(key, existingTypeId))
            return existingTypeId;

        TileDefinition def{};
        def.TypeId = defs.GetNextTypeId();
        def.Name = tile.name;
        def.UV = tile.UV;
        def.Category = tile.Category;
        def.Direction = tile.TileDirection;
        def.Material = tile.Material;
        def.BaseHealth = static_cast<uint16_t>(tile.TileHealth);
        def.IsDestructible = tile.IsDestructible;
        def.IsSupportingRoof = tile.IsSupportingRoof;
        def.IsRoof = tile.IsRoof;

        if (!defs.Register(def, key))
        {
            EE_CORE_WARN("GetOrCreateDefinitionForRuntimeTile: failed to register '{}'", tile.name);
            return 0;
        }

        return def.TypeId;
    }

    bool CompactTilePromotion::CompactEntityToTiles(Scene* scene, Entity entity, bool destroyOriginalEntity)
    {
        if (!scene)
            return false;

        if (!entity)
            return false;

        if (!entity.HasComponent<TransformComponent>() || !entity.HasComponent<TileComponent>())
            return false;

        TransformComponent& tr = entity.GetComponent<TransformComponent>();
        TileComponent& tc = entity.GetComponent<TileComponent>();

        if (tc.tiles.empty())
            return false;

        const uint64_t groupId = static_cast<uint64_t>(entity.GetUUID());
        CompactTileMap& compactMap = scene->GetCompactTileMap();

        bool wroteAnyTile = false;

        for (const TileInfo& tile : tc.tiles)
        {
            const glm::vec2 worldPos = glm::vec2(tr.Translation) + glm::vec2(tile.position);
            const glm::ivec2 worldCell = IsoTileUtils::WorldToIsoCellInt(worldPos);

            const uint16_t typeId = GetOrCreateDefinitionForRuntimeTile(scene, tile);
            if (typeId == 0)
            {
                EE_CORE_WARN("CompactEntityToTiles: failed to get type for tile '{}'", tile.name);
                continue;
            }

            // Do not duplicate same TypeId in the same cell
            if (compactMap.HasTileType(worldCell, typeId))
            {
                CompactTile* existing = compactMap.FindTile(worldCell, typeId);
                if (existing)
                {
                    existing->Flags = CompactTileFlags::None;
                    existing->Aux = 0;
                    existing->GroupId = groupId;
                }
            }
            else
            {
                CompactTile compact{};
                compact.TypeId = typeId;
                compact.Flags = CompactTileFlags::None;
                compact.Aux = 0;
                compact.GroupId = groupId;

                compactMap.AddTile(worldCell, compact);
            }

            compactMap.RegisterCellForGroup(groupId, worldCell);
            compactMap.MarkChunkDirtyForCell(worldCell);
            wroteAnyTile = true;
        }

        if (destroyOriginalEntity)
        {
            scene->DestroyEntity(entity);
        }

        return wroteAnyTile;
    }

    bool CompactTilePromotion::IsGroupPromoted(uint64_t groupId)
    {
        return m_PromotedEntitiesByGroup.find(groupId) != m_PromotedEntitiesByGroup.end();
    }

    

    void CompactTilePromotion::UnregisterPromotedEntity(uint64_t groupId)
    {
        m_PromotedEntitiesByGroup.erase(groupId);
    }

    Entity CompactTilePromotion::GetPromotedEntity(uint64_t groupId)
    {
        auto it = m_PromotedEntitiesByGroup.find(groupId);
        if (it == m_PromotedEntitiesByGroup.end())
            return {};

        return it->second;
    }


    void CompactTilePromotion::RemoveSingleTileFromExistingGroup(Scene* scene, uint64_t groupId,  const glm::ivec2& worldCell,
        Ref<TileManager> tileManager, bool destroyIfEmpty)
    {
        if (!scene || groupId == 0)
            return;

        Entity e = GetPromotedEntity(groupId);
        if (!e || !e.HasComponent<TransformComponent>() || !e.HasComponent<TileComponent>())
            return;

        TransformComponent& tr = e.GetComponent<TransformComponent>();
        TileComponent& tc = e.GetComponent<TileComponent>();


        /*
        CompactTileMap& compactMap = scene->GetCompactTileMap();

        CompactTile* compact = compactMap.GetTile(worldCell);
        if (compact)
        {
            compact->Flags &= ~CompactTileFlags::Promoted;
            compact->Flags &= ~CompactTileFlags::Hidden;
            compactMap.MarkChunkDirtyForCell(worldCell);
        }
        */

        const glm::vec2 tileWorldPos = IsoTileUtils::IsoToWorldGround(worldCell);
        const glm::vec2 localTarget = tileWorldPos - glm::vec2(tr.Translation);

        auto it = std::find_if(tc.tiles.begin(), tc.tiles.end(),
            [&](const TileInfo& t)
            {
                return std::abs(t.position.x - localTarget.x) < 0.001f &&
                    std::abs(t.position.y - localTarget.y) < 0.001f;
            });

        if (it != tc.tiles.end())
        {
            // Optional if your TileManager has unregister functions
            //tileManager->UnregisterPromotedTile(e, *it);
            // tileManager->UnregisterPromotedTileResidency(scene, e, *it, glm::vec2(tr.Translation));

            tc.tiles.erase(it);
        }

        if (tc.tiles.empty() && destroyIfEmpty)
        {
            scene->DestroyEntity(e);
        }
    }

 

    void CompactTilePromotion::EnsurePromotedAndCompactedAroundPlayer(
        Scene* scene,
        const glm::vec2& playerWorldPos,
        float promoteRadiusWorld,
        float compactRadiusWorld,
        Ref<TileManager> tileManager)
    {
        EE_PROFILE_FUNCTION();

        if (!scene)
            return;

        if (compactRadiusWorld < promoteRadiusWorld)
            compactRadiusWorld = promoteRadiusWorld;

        CompactTileMap& compactMap = scene->GetCompactTileMap();

        const float promoteRadiusSq = promoteRadiusWorld * promoteRadiusWorld;
        const float compactRadiusSq = compactRadiusWorld * compactRadiusWorld;

        std::unordered_set<uint64_t> groupsToPromote;

        // ---------------------------------------------
        // Only scan chunks near the player
        // ---------------------------------------------
        const glm::ivec2 playerCell = IsoTileUtils::WorldToIsoCell(playerWorldPos);

        const float approxCells = std::max(1.0f, compactRadiusWorld / std::max(1.0f, static_cast<float>(TILE_SIZE)));
        const int cellRadius = (int)std::ceil(approxCells);

        const int chunkRadiusX = (int)std::ceil(float(cellRadius) / float(TILE_CHUNK_W));
        const int chunkRadiusY = (int)std::ceil(float(cellRadius) / float(TILE_CHUNK_H));

        const glm::ivec2 playerChunk = WorldCellToChunkCoord(playerCell);

        for (int cy = playerChunk.y - chunkRadiusY; cy <= playerChunk.y + chunkRadiusY; ++cy)
        {
            for (int cx = playerChunk.x - chunkRadiusX; cx <= playerChunk.x + chunkRadiusX; ++cx)
            {
                const glm::ivec2 chunkCoord(cx, cy);
                const TileChunk* chunk = compactMap.GetChunk(chunkCoord);
                if (!chunk)
                    continue;

                const glm::ivec2 chunkOriginCell = ChunkCoordToWorldOrigin(chunkCoord);

                for (int y = 0; y < TILE_CHUNK_H; ++y)
                {
                    for (int x = 0; x < TILE_CHUNK_W; ++x)
                    {
                        const glm::ivec2 worldCell = chunkOriginCell + glm::ivec2(x, y);
                        const std::vector<CompactTile>* tiles = compactMap.GetTiles(worldCell);
                        if (!tiles || tiles->empty())
                            continue;

                        const glm::vec2 tileWorldPos = IsoTileUtils::IsoToWorldGround(worldCell);
                        const float distSq = glm::length2(tileWorldPos - playerWorldPos);

                        if (distSq > promoteRadiusSq)
                            continue;

                        for (const CompactTile& compact : *tiles)
                        {
                            if (compact.IsEmpty())
                                continue;

                            if (compact.GroupId == 0)
                                continue;

                            groupsToPromote.insert(compact.GroupId);
                        }
                    }
                }
            }
        }

        // ---------------------------------------------
        // Promote groups close enough
        // ---------------------------------------------
        std::unordered_set<uint64_t> groupsPromotedThisUpdate;

        for (uint64_t groupId : groupsToPromote)
        {
            if (!IsGroupPromoted(groupId))
            {
                Entity e = PromoteGroup(scene, groupId);
                if (e)
                {
                    SortEntitiesByY(scene);
                    SortEntityTilesByY(scene, e);

                    tileManager->RegisterPromotedEntity(e);
                    tileManager->RegisterPromotedEntityResidency(scene, e, playerWorldPos);
                    groupsPromotedThisUpdate.insert(groupId);
                }
            }
        }

        // ---------------------------------------------
        // Compact back promoted groups far away
        // ---------------------------------------------
        std::vector<uint64_t> groupsToCompact;
        groupsToCompact.reserve(m_PromotedEntitiesByGroup.size());

        for (const auto& [groupId, entity] : m_PromotedEntitiesByGroup)
        {
            if (groupsPromotedThisUpdate.count(groupId))
                continue;

            if (!entity)
            {
                groupsToCompact.push_back(groupId);
                continue;
            }

            if (!entity.HasComponent<TransformComponent>())
            {
                groupsToCompact.push_back(groupId);
                continue;
            }

            const TransformComponent& tr = entity.GetComponent<TransformComponent>();
            const glm::vec2 entityWorldPos = glm::vec2(tr.Translation);
            const float distSq = glm::length2(entityWorldPos - playerWorldPos);

            if (distSq > compactRadiusSq)
            {
                groupsToCompact.push_back(groupId);
            }
        }

        for (uint64_t groupId : groupsToCompact)
        {
            Entity e = GetPromotedEntity(groupId);
            if (!e)
            {
                UnregisterPromotedEntity(groupId);
                continue;
            }

            CompactEntityToTiles(scene, e, true);
        }
    }

    
    void CompactTilePromotion::SortEntitiesByY(Scene* scene)
    {
        auto& reg = scene->GetRegistry();

        reg.sort<TransformComponent>(
            [&reg](entt::entity a, entt::entity b)
            {
                const auto& ta = reg.get<TransformComponent>(a).Translation;
                const auto& tb = reg.get<TransformComponent>(b).Translation;

                if (ta.y != tb.y)
                    return ta.y > tb.y;

                using underlying = std::underlying_type_t<entt::entity>;
                return static_cast<underlying>(a) < static_cast<underlying>(b);
            }
        );
    }


    void CompactTilePromotion::SortEntityTilesByY(Scene* scene, Entity entity)
    {
        if (!entity || !entity.HasComponent<TileComponent>() || !entity.HasComponent<TransformComponent>())
            return;

        auto& tileComp = entity.GetComponent<TileComponent>();
        const auto& tr = entity.GetComponent<TransformComponent>();

        auto DirectionSortKey = [](eTileDirection dir) -> int
            {
                switch (dir)
                {
                case eTileDirection::North: return 0;
                case eTileDirection::East:  return 1;
                case eTileDirection::West:  return 0;
                case eTileDirection::South: return 0;
                default: return 100;
                }
            };

        std::stable_sort(tileComp.tiles.begin(), tileComp.tiles.end(),
            [&](const TileInfo& A, const TileInfo& B)
            {
                const float yA = tr.Translation.y + A.position.y;
                const float yB = tr.Translation.y + B.position.y;

                if (yA > yB) return true;
                if (yA < yB) return false;

                const float xA = tr.Translation.x + A.position.x;
                const float xB = tr.Translation.x + B.position.x;

                if (xA < xB) return true;
                if (xA > xB) return false;

                const int dirA = DirectionSortKey(A.TileDirection);
                const int dirB = DirectionSortKey(B.TileDirection);

                if (dirA != dirB)
                    return dirA < dirB;

                return false;
            }
        );
    }

    void CompactTilePromotion::EnsurePromotedInEditorViewport(Scene* scene, const glm::vec2& viewMinWorld,
        const glm::vec2& viewMaxWorld, float compactMarginWorld, Ref<TileManager> tileManager)
    {
        EE_PROFILE_FUNCTION();

        if (!scene)
            return;

        CompactTileMap& compactMap = scene->GetCompactTileMap();

        const glm::vec2 promoteMin = viewMinWorld;
        const glm::vec2 promoteMax = viewMaxWorld;

        const glm::vec2 compactMin = viewMinWorld - glm::vec2(compactMarginWorld);
        const glm::vec2 compactMax = viewMaxWorld + glm::vec2(compactMarginWorld);

        auto pointInRect = [](const glm::vec2& p, const glm::vec2& rmin, const glm::vec2& rmax) -> bool
            {
                return p.x >= rmin.x && p.x <= rmax.x &&
                    p.y >= rmin.y && p.y <= rmax.y;
            };

        std::unordered_set<uint64_t> groupsToPromote;
        std::unordered_set<uint64_t> groupsPromotedThisUpdate;

        // -------------------------------------------------
        // Convert viewport corners to approximate chunk range
        // -------------------------------------------------
        const glm::ivec2 cellMin = IsoTileUtils::WorldToIsoCell(promoteMin);
        const glm::ivec2 cellMax = IsoTileUtils::WorldToIsoCell(promoteMax);

        const int minCellX = std::min(cellMin.x, cellMax.x) - TILE_CHUNK_W;
        const int maxCellX = std::max(cellMin.x, cellMax.x) + TILE_CHUNK_W;
        const int minCellY = std::min(cellMin.y, cellMax.y) - TILE_CHUNK_H;
        const int maxCellY = std::max(cellMin.y, cellMax.y) + TILE_CHUNK_H;

        const glm::ivec2 minChunk = WorldCellToChunkCoord({ minCellX, minCellY });
        const glm::ivec2 maxChunk = WorldCellToChunkCoord({ maxCellX, maxCellY });
      
        // -------------------------------------------------
        // Pass 1: collect groups from compact tiles inside viewport
        // -------------------------------------------------
        // -------------------------------------------------
// Pass 1: collect groups from compact tiles inside viewport
// -------------------------------------------------
        for (int cy = minChunk.y; cy <= maxChunk.y; ++cy)
        {
            for (int cx = minChunk.x; cx <= maxChunk.x; ++cx)
            {
                const glm::ivec2 chunkCoord(cx, cy);
                const TileChunk* chunk = compactMap.GetChunk(chunkCoord);
                if (!chunk)
                    continue;

                const glm::ivec2 chunkOriginCell = ChunkCoordToWorldOrigin(chunkCoord);

                for (int y = 0; y < TILE_CHUNK_H; ++y)
                {
                    for (int x = 0; x < TILE_CHUNK_W; ++x)
                    {
                        const glm::ivec2 worldCell = chunkOriginCell + glm::ivec2(x, y);
                        const std::vector<CompactTile>* tiles = compactMap.GetTiles(worldCell);
                        if (!tiles || tiles->empty())
                            continue;

                        const glm::vec2 worldPos = IsoTileUtils::IsoToWorldGround(worldCell);
                        if (!pointInRect(worldPos, promoteMin, promoteMax))
                            continue;

                        for (const CompactTile& ct : *tiles)
                        {
                            if (ct.IsEmpty())
                                continue;

                            if (ct.GroupId == 0)
                                continue;

                            if (ct.Flags & CompactTileFlags::Promoted)
                                continue;

                            groupsToPromote.insert(ct.GroupId);
                        }
                    }
                }
            }
        }

        // -------------------------------------------------
        // Pass 2: promote groups
        // -------------------------------------------------
        for (uint64_t groupId : groupsToPromote)
        {
            if (!IsGroupPromoted(groupId))
            {
                Entity e = PromoteGroup(scene, groupId);
                if (e && scene->IsEntityValid(e))
                {
                    RegisterPromotedEntity(groupId, e);
                    tileManager->RegisterPromotedEntity(e);
                    tileManager->RegisterPromotedEntityResidency(scene, e, 0.5f * (promoteMin + promoteMax));
                    groupsPromotedThisUpdate.insert(groupId);
                    EE_CORE_INFO("PromoteGroup created entity {} for group {}", (uint64_t)e.GetUUID(), groupId);
                }
            }
        }

        // -------------------------------------------------
        // Pass 3: compact back promoted groups outside expanded rect
        // -------------------------------------------------
        std::vector<uint64_t> groupsToCompact;
        groupsToCompact.reserve(m_PromotedEntitiesByGroup.size());

        for (auto& [groupId, entity] : m_PromotedEntitiesByGroup)
        {
            if (groupsPromotedThisUpdate.count(groupId))
                continue;

            if (!entity)
            {
                groupsToCompact.push_back(groupId);
                continue;
            }

            if (!entity.HasComponent<TransformComponent>())
            {
                groupsToCompact.push_back(groupId);
                continue;
                
            }

            const glm::vec2 entityWorldPos = glm::vec2(entity.GetComponent<TransformComponent>().Translation);

            if (!pointInRect(entityWorldPos, compactMin, compactMax))
            {
                groupsToCompact.push_back(groupId);
            }
        }

        for (uint64_t groupId : groupsToCompact)
        {
            Entity e = GetPromotedEntity(groupId);
            if (!e)
            {
                UnregisterPromotedEntity(groupId);
                continue;
            }

            CompactEntityToTiles(scene, e, true);
        }
    }
   

    void CompactTilePromotion::RegisterPromotedEntity(uint64_t groupId, Entity entity)
    {
        if (groupId == 0 || !entity)
        {
            EE_CORE_WARN("RegisterPromotedEntity invalid input groupId={} entity={}", groupId, (bool)entity);
            return;
        }

        m_PromotedEntitiesByGroup[groupId] = entity;

        EE_CORE_INFO("Registered promoted entity {} for group {}", (uint64_t)entity.GetUUID(), groupId);
    }


    bool CompactTilePromotion::PromoteSingleTileIntoExistingGroup(Scene* scene, uint64_t groupId,
        glm::ivec2& worldCell,  Ref<TileManager> tileManager, eTileDirection tileDir, uint16_t typeId)
    {
        if (!scene || groupId == 0)
            return false;

        Entity e = GetPromotedEntity(groupId);
        if (!e || !e.HasComponent<TransformComponent>() || !e.HasComponent<TileComponent>() || !e.HasComponent<IDComponent>())
            return false;

        CompactTileMap& compactMap = scene->GetCompactTileMap();
        TileDefinitionRegistry& defs = scene->GetTileDefinitions();

        CompactTile* compact = compactMap.FindTile(worldCell, typeId);
        if (!compact || compact->IsEmpty())
            return false;

        if (compact->GroupId != groupId)
            return false;

        if (compact->Flags & CompactTileFlags::Promoted)
            return true;

        const TileDefinition* def = defs.Get(compact->TypeId);
        if (!def)
            return false;

        TransformComponent& tr = e.GetComponent<TransformComponent>();
        TileComponent& tc = e.GetComponent<TileComponent>();
        IDComponent& id = e.GetComponent<IDComponent>();

        const glm::vec2 tileWorldPos = IsoTileUtils::IsoToWorldGround(worldCell);
        const glm::vec2 localPos = tileWorldPos - glm::vec2(tr.Translation);

        TileInfo runtimeTile = BuildRuntimeTileFromDefinition(*def, localPos);
        runtimeTile.UID = HashUtils::MakeTileUID(
            (uint64_t)id.ID,
            runtimeTile.position,
            float(TILE_SIZE),
            (uint32_t)runtimeTile.Category,
            tileDir);
        EE_CORE_INFO("PromoteSingleTileIntoExistingGroup UID {}", runtimeTile.UID);

        tc.tiles.push_back(runtimeTile);

        tileManager->RegisterPromotedTile(e, tc.tiles.back());
        tileManager->RegisterPromotedTileResidency(scene, e, tc.tiles.back(), glm::vec2(tr.Translation));

        compact->Flags |= CompactTileFlags::Promoted;
        compact->Flags |= CompactTileFlags::Hidden;
        compactMap.MarkChunkDirtyForCell(worldCell);

        return true;
    }

}