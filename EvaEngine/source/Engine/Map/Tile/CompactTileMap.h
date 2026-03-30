#pragma once

#include <array>
#include <unordered_map>
#include <glm/glm.hpp>
#include "TileDefinition.h"

namespace Engine
{
    static constexpr int TILE_CHUNK_W = 64;
    static constexpr int TILE_CHUNK_H = 64;

    struct CompactTile
    {
        uint16_t TypeId = 0;
        uint8_t Flags = 0;
        uint8_t Aux = 0;
        uint64_t GroupId = 0;

        bool IsEmpty() const;
    };

    namespace CompactTileFlags
    {
        static constexpr uint8_t None = 0;
        static constexpr uint8_t Hidden = 1 << 0;
        static constexpr uint8_t Promoted = 1 << 1;
    }

    struct CompactDrawItem
    {
        glm::ivec2 WorldCell{};
        uint16_t TypeId = 0;
        int Layer = 0;
        float WorldY = 0.0f;
    };


    struct TileChunk
    {
        glm::ivec2 ChunkCoord{};
        std::array<CompactTile, TILE_CHUNK_W* TILE_CHUNK_H> Tiles{};

        std::vector<CompactDrawItem> CachedDrawItems;
        bool DrawCacheDirty = true;

        CompactTile& At(int x, int y);
        const CompactTile& At(int x, int y) const;
    };

    struct IVec2Hash
    {
        size_t operator()(const glm::ivec2& v) const;
    };

    class TileDefinitionRegistry;

    class CompactTileMap
    {
    public:
        const std::unordered_map<glm::ivec2, TileChunk, IVec2Hash>& GetChunks() const;
        std::unordered_map<glm::ivec2, TileChunk, IVec2Hash>& GetChunksMutable();

        TileChunk* GetChunk(const glm::ivec2& chunkCoord);
        const TileChunk* GetChunk(const glm::ivec2& chunkCoord) const;

        TileChunk& GetOrCreateChunk(const glm::ivec2& chunkCoord);

        CompactTile* GetTile(const glm::ivec2& worldCell);
        const CompactTile* GetTile(const glm::ivec2& worldCell) const;

        CompactTile& GetOrCreateTile(const glm::ivec2& worldCell);

        void Render(const TileDefinitionRegistry& defs);

        void Render(const TileDefinitionRegistry& defs) const;
        const std::vector<glm::ivec2>* GetCellsForGroup(uint64_t groupId) const;
        void RegisterCellForGroup(uint64_t groupId, const glm::ivec2& cell);
        void RemoveCellFromGroup(uint64_t groupId, const glm::ivec2& cell);
        void RebuildChunkDrawCache(TileChunk& chunk, const TileDefinitionRegistry& defs);
        void MarkChunkDirtyForCell(const glm::ivec2& worldCell);
        const std::unordered_map<uint64_t, std::vector<glm::ivec2>>& GetGroupCells() const;
    private:
        std::unordered_map<glm::ivec2, TileChunk, IVec2Hash> m_chunks;
        std::unordered_map<uint64_t, std::vector<glm::ivec2>> m_CellsByGroupId;

    };

    int FloorDiv(int a, int b);
    int ModFloor(int a, int b);

    glm::ivec2 WorldCellToChunkCoord(const glm::ivec2& worldCell);
    glm::ivec2 WorldCellToLocalCell(const glm::ivec2& worldCell);
    glm::ivec2 ChunkCoordToWorldOrigin(const glm::ivec2& chunkCoord);
}