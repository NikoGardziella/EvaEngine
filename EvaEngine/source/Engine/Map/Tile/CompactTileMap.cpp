#include "pch.h"
#include "CompactTileMap.h"

#include <cmath>
#include <cstdlib>
#include <functional>
#include "Engine/Map/Utils/IsoTileUtils.h"

namespace Engine
{
    bool CompactTile::IsEmpty() const
    {
        return TypeId == 0;
    }

    CompactTile& TileChunk::At(int x, int y)
    {
        return Tiles[y * TILE_CHUNK_W + x];
    }

    const CompactTile& TileChunk::At(int x, int y) const
    {
        return Tiles[y * TILE_CHUNK_W + x];
    }

    size_t IVec2Hash::operator()(const glm::ivec2& v) const
    {
        size_t h = std::hash<int>{}(v.x);
        h ^= std::hash<int>{}(v.y) + 0x9e3779b9 + (h << 6) + (h >> 2);
        return h;
    }

    const std::unordered_map<glm::ivec2, TileChunk, IVec2Hash>& CompactTileMap::GetChunks() const
    {
        return m_chunks;
    }

    std::unordered_map<glm::ivec2, TileChunk, IVec2Hash>& CompactTileMap::GetChunksMutable()
    {
        return m_chunks;
    }

    TileChunk* CompactTileMap::GetChunk(const glm::ivec2& chunkCoord)
    {
        auto it = m_chunks.find(chunkCoord);
        if (it == m_chunks.end())
            return nullptr;

        return &it->second;
    }

    const TileChunk* CompactTileMap::GetChunk(const glm::ivec2& chunkCoord) const
    {
        auto it = m_chunks.find(chunkCoord);
        if (it == m_chunks.end())
            return nullptr;

        return &it->second;
    }

    TileChunk& CompactTileMap::GetOrCreateChunk(const glm::ivec2& chunkCoord)
    {
        auto [it, inserted] = m_chunks.emplace(chunkCoord, TileChunk{});
        it->second.ChunkCoord = chunkCoord;
        return it->second;
    }

    CompactTile* CompactTileMap::GetTile(const glm::ivec2& worldCell)
    {
        const glm::ivec2 chunk = WorldCellToChunkCoord(worldCell);
        const glm::ivec2 local = WorldCellToLocalCell(worldCell);

        TileChunk* c = GetChunk(chunk);
        if (!c)
            return nullptr;

        return &c->At(local.x, local.y);
    }

    const CompactTile* CompactTileMap::GetTile(const glm::ivec2& worldCell) const
    {
        const glm::ivec2 chunk = WorldCellToChunkCoord(worldCell);
        const glm::ivec2 local = WorldCellToLocalCell(worldCell);

        const TileChunk* c = GetChunk(chunk);
        if (!c)
            return nullptr;

        return &c->At(local.x, local.y);
    }

    CompactTile& CompactTileMap::GetOrCreateTile(const glm::ivec2& worldCell)
    {
        const glm::ivec2 chunk = WorldCellToChunkCoord(worldCell);
        const glm::ivec2 local = WorldCellToLocalCell(worldCell);

        TileChunk& c = GetOrCreateChunk(chunk);
        return c.At(local.x, local.y);
    }

    void CompactTileMap::Render(const TileDefinitionRegistry& defs)
    {
        for (auto& [chunkCoord, chunk] : m_chunks)
        {
            if (chunk.DrawCacheDirty)
            {
                RebuildChunkDrawCache(chunk, defs);
            }

            for (const CompactDrawItem& item : chunk.CachedDrawItems)
            {
                const TileDefinition* def = defs.Get(item.TypeId);
                if (!def)
                    continue;

                const glm::vec2 worldPos = IsoTileUtils::IsoToWorldGround(item.WorldCell);

                float zBias = 0.0f;
                uint32_t flags = 0;

                if (def->Category == eTileCategory::Roofs)
                {
                    zBias = 2.0f;
                    flags |= VulkanBindlessDescriptorSetRenderer::eTileFlags::IsRoof;
                }

                glm::vec4 color = glm::vec4(1.0f);
                VulkanRenderer2D::DrawTile(worldPos, def->UV, color);
            }
        }
    }

    const std::vector<glm::ivec2>* CompactTileMap::GetCellsForGroup(uint64_t groupId) const
    {
        auto it = m_CellsByGroupId.find(groupId);
        if (it == m_CellsByGroupId.end())
            return nullptr;

        return &it->second;
    }

    void CompactTileMap::RegisterCellForGroup(uint64_t groupId, const glm::ivec2& cell)
    {
        if (groupId == 0)
            return;

        auto& vec = m_CellsByGroupId[groupId];

        // Avoid duplicates
        for (const glm::ivec2& c : vec)
        {
            if (c == cell)
                return;
        }

        vec.push_back(cell);
    }

    void CompactTileMap::RemoveCellFromGroup(uint64_t groupId, const glm::ivec2& cell)
    {
        auto it = m_CellsByGroupId.find(groupId);
        if (it == m_CellsByGroupId.end())
            return;

        auto& vec = it->second;

        for (size_t i = 0; i < vec.size(); ++i)
        {
            if (vec[i] == cell)
            {
                vec[i] = vec.back();
                vec.pop_back();
                break;
            }
        }

        // Optional: clean empty groups
        if (vec.empty())
        {
            m_CellsByGroupId.erase(it);
        }
    }
   

    void CompactTileMap::RebuildChunkDrawCache(TileChunk& chunk, const TileDefinitionRegistry& defs)
    {
        chunk.CachedDrawItems.clear();

        const glm::ivec2 chunkOriginCell = ChunkCoordToWorldOrigin(chunk.ChunkCoord);

        for (int y = 0; y < TILE_CHUNK_H; ++y)
        {
            for (int x = 0; x < TILE_CHUNK_W; ++x)
            {
                const CompactTile& ct = chunk.At(x, y);

                if (ct.IsEmpty())
                    continue;

                if (ct.Flags & CompactTileFlags::Hidden)
                    continue;

                if (ct.Flags & CompactTileFlags::Promoted)
                    continue;

                const TileDefinition* def = defs.Get(ct.TypeId);
                if (!def)
                    continue;

                const glm::ivec2 worldCell = chunkOriginCell + glm::ivec2(x, y);
                const glm::vec2 worldPos = IsoTileUtils::IsoToWorldGround(worldCell);

                int layer = 0;
                if (def->Category == eTileCategory::Roofs) layer = 2;
                else if (def->Category == eTileCategory::Buildings) layer = 1;

                CompactDrawItem item{};
                item.WorldCell = worldCell;
                item.TypeId = ct.TypeId;
                item.Layer = layer;
                item.WorldY = worldPos.y;

                chunk.CachedDrawItems.push_back(item);
            }
        }

        std::stable_sort(chunk.CachedDrawItems.begin(), chunk.CachedDrawItems.end(),
            [](const CompactDrawItem& a, const CompactDrawItem& b)
            {
                if (a.Layer != b.Layer)
                    return a.Layer < b.Layer;

                if (a.WorldY != b.WorldY)
                    return a.WorldY > b.WorldY;

                if (a.WorldCell.x != b.WorldCell.x)
                    return a.WorldCell.x < b.WorldCell.x;

                return a.WorldCell.y < b.WorldCell.y;
            });

        chunk.DrawCacheDirty = false;
    }

    void CompactTileMap::MarkChunkDirtyForCell(const glm::ivec2& worldCell)
    {
        const glm::ivec2 chunkCoord = WorldCellToChunkCoord(worldCell);
        TileChunk& chunk = GetOrCreateChunk(chunkCoord);
        chunk.DrawCacheDirty = true;
    }


    const std::unordered_map<uint64_t, std::vector<glm::ivec2>>& CompactTileMap::GetGroupCells() const
    {
        return m_CellsByGroupId;
    }

    int FloorDiv(int a, int b)
    {
        int q = a / b;
        int r = a % b;

        if ((r != 0) && ((r < 0) != (b < 0)))
            --q;

        return q;
    }

    int ModFloor(int a, int b)
    {
        int m = a % b;
        if (m < 0)
            m += std::abs(b);

        return m;
    }

    glm::ivec2 WorldCellToChunkCoord(const glm::ivec2& worldCell)
    {
        return glm::ivec2(
            FloorDiv(worldCell.x, TILE_CHUNK_W),
            FloorDiv(worldCell.y, TILE_CHUNK_H)
        );
    }

    glm::ivec2 WorldCellToLocalCell(const glm::ivec2& worldCell)
    {
        return glm::ivec2(
            ModFloor(worldCell.x, TILE_CHUNK_W),
            ModFloor(worldCell.y, TILE_CHUNK_H)
        );
    }

    glm::ivec2 ChunkCoordToWorldOrigin(const glm::ivec2& chunkCoord)
    {
        return glm::ivec2(
            chunkCoord.x * TILE_CHUNK_W,
            chunkCoord.y * TILE_CHUNK_H
        );
    }
}