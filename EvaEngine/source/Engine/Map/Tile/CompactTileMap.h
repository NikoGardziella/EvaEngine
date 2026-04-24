#pragma once

#include <unordered_map>
#include <vector>
#include <glm/glm.hpp>
#include "TileDefinition.h"

namespace Engine
{
    inline constexpr int TILE_CHUNK_W = 64;
    inline constexpr int TILE_CHUNK_H = 64;

    struct CompactGroupInfo
    {
        uint64_t GroupId = 0;
        glm::ivec2 OriginCell{};
        std::string Name;
    };

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
        inline constexpr uint8_t None = 0;
        inline constexpr uint8_t Hidden = 1 << 0;
        inline constexpr uint8_t Promoted = 1 << 1;
    }

    struct CompactTileCell
    {
        std::vector<CompactTile> Tiles;
    };

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

        // Cached render data for compact tiles that fall inside this chunk
        std::vector<CompactDrawItem> CachedDrawItems;
        bool DrawCacheDirty = true;
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

        std::vector<CompactTile>* GetTiles(const glm::ivec2& worldCell);
        const std::vector<CompactTile>* GetTiles(const glm::ivec2& worldCell) const;
        std::vector<CompactTile>& GetOrCreateTiles(const glm::ivec2& worldCell);

        CompactTile* FindTile(const glm::ivec2& worldCell, uint16_t typeId);
        const CompactTile* FindTile(const glm::ivec2& worldCell, uint16_t typeId) const;

        bool HasTileType(const glm::ivec2& worldCell, uint16_t typeId) const;
        CompactTile& AddTile(const glm::ivec2& worldCell, const CompactTile& tile);
        bool RemoveTile(const glm::ivec2& worldCell, uint16_t typeId);

        void Render(const TileDefinitionRegistry& defs) const;


        const std::vector<glm::ivec2>* GetCellsForGroup(uint64_t groupId) const;
        void RegisterCellForGroup(uint64_t groupId, const glm::ivec2& cell);
        void RemoveCellFromGroup(uint64_t groupId, const glm::ivec2& cell);

        const std::unordered_map<uint64_t, CompactGroupInfo>& GetAllGroupInfo() const;

        void RemoveGroup(uint64_t groupId);
        void ClearPromotionFlags();
        void ClearPromotionFlagsForGroup(uint64_t groupId);
        void Clear();
        bool RemoveTileByCategoryAndDirection(const glm::ivec2& worldCell, const TileDefinitionRegistry& defs, eTileCategory category, eTileDirection direction);
        void RebuildChunkDrawCache(TileChunk& chunk, const TileDefinitionRegistry& defs);
        void MarkChunkDirtyForCell(const glm::ivec2& worldCell);

        const std::unordered_map<uint64_t, std::vector<glm::ivec2>>& GetGroupCells() const;


        const CompactGroupInfo* GetGroupInfo(uint64_t groupId) const;
        CompactGroupInfo* GetGroupInfo(uint64_t groupId);
        void SetGroupOrigin(uint64_t groupId, const glm::ivec2& originCell);
        void SetGroupName(uint64_t groupId, const std::string& groupName);
        bool HasGroupInfo(uint64_t groupId) const;
        void RemoveGroupInfo(uint64_t groupId);


    private:
        std::unordered_map<glm::ivec2, TileChunk, IVec2Hash> m_Chunks;
        std::unordered_map<glm::ivec2, CompactTileCell, IVec2Hash> m_Cells;
        std::unordered_map<uint64_t, std::vector<glm::ivec2>> m_CellsByGroupId;
        std::unordered_map<uint64_t, CompactGroupInfo> m_GroupInfo;
    };

    int FloorDiv(int a, int b);
    int ModFloor(int a, int b);

    glm::ivec2 WorldCellToChunkCoord(const glm::ivec2& worldCell);
    glm::ivec2 WorldCellToLocalCell(const glm::ivec2& worldCell);
    glm::ivec2 ChunkCoordToWorldOrigin(const glm::ivec2& chunkCoord);
}