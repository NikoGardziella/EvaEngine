#pragma once

#include <unordered_map>
#include "CompactTileMap.h"
#include "TileManager.h" 
#include "TileDefinition.h"

namespace Engine
{
    class TileDefinitionRegistry
    {
    public:
        bool Register(const TileDefinition& def, const TileTypeKey& key);
        const TileDefinition* Get(uint16_t typeId) const;
        TileDefinition* GetMutable(uint16_t typeId);
        bool Has(uint16_t typeId) const;
        void Clear();

        bool FindTypeId(const TileTypeKey& key, uint16_t& outTypeId) const;
        uint16_t GetNextTypeId();

        const std::unordered_map<uint16_t, TileDefinition>& GetDefinitions() const { return m_definitions; }

    private:
        std::unordered_map<uint16_t, TileDefinition> m_definitions;
        std::unordered_map<TileTypeKey, uint16_t, TileTypeKeyHash> m_typeIdByKey;
        uint16_t m_nextTypeId = 1;
    };
}