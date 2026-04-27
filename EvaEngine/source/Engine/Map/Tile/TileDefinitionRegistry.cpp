#include "pch.h"
#include "TileDefinitionRegistry.h"

namespace Engine
{
    bool TileDefinitionRegistry::Register(const TileDefinition& def, const TileTypeKey& key)
    {
        if (key.category == eTileCategory::Undefined)
        {
            EE_CORE_ERROR("wrong category");
        }
        if (def.TypeId == 0)
            return false;

        m_definitions[def.TypeId] = def;
        m_typeIdByKey[key] = def.TypeId;

        if (def.TypeId >= m_nextTypeId)
            m_nextTypeId = def.TypeId + 1;

        return true;
    }

    uint16_t TileDefinitionRegistry::GetTypeIdByName(const std::string& name) const
    {
        for (const auto& [id, def] : m_definitions)
        {
            if (def.Name == name)
                return id;
        }

        return 0;
    }

    const TileDefinition* TileDefinitionRegistry::Get(uint16_t typeId) const
    {
        auto it = m_definitions.find(typeId);
        if (it == m_definitions.end())
            return nullptr;
        return &it->second;
    }

    TileDefinition* TileDefinitionRegistry::GetMutable(uint16_t typeId)
    {
        auto it = m_definitions.find(typeId);
        if (it == m_definitions.end())
            return nullptr;
        return &it->second;
    }

    bool TileDefinitionRegistry::Has(uint16_t typeId) const
    {
        return m_definitions.find(typeId) != m_definitions.end();
    }

    void TileDefinitionRegistry::Clear()
    {
        m_definitions.clear();
        m_typeIdByKey.clear();
        m_nextTypeId = 1;
    }

    bool TileDefinitionRegistry::FindTypeId(const TileTypeKey& key, uint16_t& outTypeId) const
    {

        auto it = m_typeIdByKey.find(key);
        if (it == m_typeIdByKey.end())
            return false;

        outTypeId = it->second;
        return true;
    }

    uint16_t TileDefinitionRegistry::GetNextTypeId()
    {
        return m_nextTypeId++;
    }
}