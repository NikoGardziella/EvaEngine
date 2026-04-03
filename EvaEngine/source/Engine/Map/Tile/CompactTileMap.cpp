#include "pch.h"
#include "CompactTileMap.h"

#include <algorithm>
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

    size_t IVec2Hash::operator()(const glm::ivec2& v) const
    {
        size_t h = std::hash<int>{}(v.x);
        h ^= std::hash<int>{}(v.y) + 0x9e3779b9 + (h << 6) + (h >> 2);
        return h;
    }

    const std::unordered_map<glm::ivec2, TileChunk, IVec2Hash>& CompactTileMap::GetChunks() const
    {
        return m_Chunks;
    }

    std::unordered_map<glm::ivec2, TileChunk, IVec2Hash>& CompactTileMap::GetChunksMutable()
    {
        return m_Chunks;
    }

    TileChunk* CompactTileMap::GetChunk(const glm::ivec2& chunkCoord)
    {
        auto it = m_Chunks.find(chunkCoord);
        if (it == m_Chunks.end())
            return nullptr;

        return &it->second;
    }

    const TileChunk* CompactTileMap::GetChunk(const glm::ivec2& chunkCoord) const
    {
        auto it = m_Chunks.find(chunkCoord);
        if (it == m_Chunks.end())
            return nullptr;

        return &it->second;
    }

    TileChunk& CompactTileMap::GetOrCreateChunk(const glm::ivec2& chunkCoord)
    {
        auto [it, inserted] = m_Chunks.emplace(chunkCoord, TileChunk{});
        it->second.ChunkCoord = chunkCoord;
        return it->second;
    }

    std::vector<CompactTile>* CompactTileMap::GetTiles(const glm::ivec2& worldCell)
    {
        auto it = m_Cells.find(worldCell);
        if (it == m_Cells.end())
            return nullptr;

        return &it->second.Tiles;
    }

    const std::vector<CompactTile>* CompactTileMap::GetTiles(const glm::ivec2& worldCell) const
    {
        auto it = m_Cells.find(worldCell);
        if (it == m_Cells.end())
            return nullptr;

        return &it->second.Tiles;
    }

    std::vector<CompactTile>& CompactTileMap::GetOrCreateTiles(const glm::ivec2& worldCell)
    {
        return m_Cells[worldCell].Tiles;
    }

    CompactTile* CompactTileMap::FindTile(const glm::ivec2& worldCell, uint16_t typeId)
    {
        auto it = m_Cells.find(worldCell);
        if (it == m_Cells.end())
            return nullptr;

        auto& tiles = it->second.Tiles;
        auto found = std::find_if(tiles.begin(), tiles.end(),
            [&](CompactTile& t)
            {
                return t.TypeId == typeId;
            });

        return (found != tiles.end()) ? &(*found) : nullptr;
    }

    const CompactTile* CompactTileMap::FindTile(const glm::ivec2& worldCell, uint16_t typeId) const
    {
        auto it = m_Cells.find(worldCell);
        if (it == m_Cells.end())
            return nullptr;

        const auto& tiles = it->second.Tiles;
        auto found = std::find_if(tiles.begin(), tiles.end(),
            [&](const CompactTile& t)
            {
                return t.TypeId == typeId;
            });

        return (found != tiles.end()) ? &(*found) : nullptr;
    }

    bool CompactTileMap::HasTileType(const glm::ivec2& worldCell, uint16_t typeId) const
    {
        return FindTile(worldCell, typeId) != nullptr;
    }

    CompactTile& CompactTileMap::AddTile(const glm::ivec2& worldCell, const CompactTile& tile)
    {
        CompactTileCell& cell = m_Cells[worldCell];

        auto it = std::find_if(cell.Tiles.begin(), cell.Tiles.end(),
            [&](const CompactTile& t)
            {
                return t.TypeId == tile.TypeId;
            });

        if (it != cell.Tiles.end())
        {
            return *it;
        }

        cell.Tiles.push_back(tile);
        MarkChunkDirtyForCell(worldCell);
        return cell.Tiles.back();
    }

    bool CompactTileMap::RemoveTile(const glm::ivec2& worldCell, uint16_t typeId)
    {
        auto it = m_Cells.find(worldCell);
        if (it == m_Cells.end())
            return false;

        auto& tiles = it->second.Tiles;

        auto found = std::find_if(tiles.begin(), tiles.end(),
            [&](const CompactTile& t)
            {
                return t.TypeId == typeId;
            });

        if (found == tiles.end())
            return false;

        tiles.erase(found);

        if (tiles.empty())
            m_Cells.erase(it);

        MarkChunkDirtyForCell(worldCell);
        return true;
    }

    void CompactTileMap::Render(const TileDefinitionRegistry& defs) const
    {
        for (const auto& [chunkCoord, chunk] : m_Chunks)
        {
            for (const CompactDrawItem& item : chunk.CachedDrawItems)
            {
                const TileDefinition* def = defs.Get(item.TypeId);
                if (!def)
                    continue;

                const glm::vec2 worldPos = IsoTileUtils::IsoToWorldGround(item.WorldCell);
                const glm::vec4 color = glm::vec4(1.0f);

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

        auto& cells = m_CellsByGroupId[groupId];

        auto it = std::find(cells.begin(), cells.end(), cell);
        if (it == cells.end())
            cells.push_back(cell);
    }

    void CompactTileMap::RemoveCellFromGroup(uint64_t groupId, const glm::ivec2& cell)
    {
        auto it = m_CellsByGroupId.find(groupId);
        if (it == m_CellsByGroupId.end())
            return;

        auto& cells = it->second;
        auto found = std::find(cells.begin(), cells.end(), cell);
        if (found != cells.end())
            cells.erase(found);

        if (cells.empty())
            m_CellsByGroupId.erase(it);
    }

    void CompactTileMap::RebuildChunkDrawCache(TileChunk& chunk, const TileDefinitionRegistry& defs)
    {
        chunk.CachedDrawItems.clear();

        const glm::ivec2 chunkOriginCell = ChunkCoordToWorldOrigin(chunk.ChunkCoord);

        for (int y = 0; y < TILE_CHUNK_H; ++y)
        {
            for (int x = 0; x < TILE_CHUNK_W; ++x)
            {
                const glm::ivec2 worldCell = chunkOriginCell + glm::ivec2(x, y);
                const std::vector<CompactTile>* tiles = GetTiles(worldCell);
                if (!tiles || tiles->empty())
                    continue;

                const glm::vec2 worldPos = IsoTileUtils::IsoToWorldGround(worldCell);

                for (const CompactTile& ct : *tiles)
                {
                    if (ct.IsEmpty())
                        continue;

                    if (ct.Flags & CompactTileFlags::Hidden)
                        continue;

                    if (ct.Flags & CompactTileFlags::Promoted)
                        continue;

                    const TileDefinition* def = defs.Get(ct.TypeId);
                    if (!def)
                        continue;

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

                if (a.WorldCell.y != b.WorldCell.y)
                    return a.WorldCell.y < b.WorldCell.y;

                return a.TypeId < b.TypeId;
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