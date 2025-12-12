#include "pch.h"
#include "MeshRegistry.h"

namespace Engine {

    const MeshAsset& MeshRegistry::GetMesh(uint32_t meshId) const 
    {
        return m_meshes.at(meshId);
    }

    MeshAsset& MeshRegistry::GetMesh(uint32_t meshId)
    {
        return m_meshes.at(meshId);
    }

    MeshId MeshRegistry::FindMeshId(const std::string& key) const
    {
        auto it = m_keyToId.find(key);
        return (it != m_keyToId.end()) ? it->second : INVALID_MESH;
    }

    const MeshAsset* MeshRegistry::GetMeshByKey(const std::string& key) const
    {
        auto it = m_keyToId.find(key);
        if (it == m_keyToId.end())
            return nullptr;

        return &m_meshes[it->second];
    }


    MeshId MeshRegistry::RegisterMesh(const std::string& key, const MeshAsset& asset)
    {
        auto it = m_keyToId.find(key);
        if (it != m_keyToId.end())
            return it->second;

        MeshId id = (MeshId)m_meshes.size();
        m_meshes.push_back(asset);
        m_keyToId[key] = id;
        EE_CORE_INFO("registered mesh {}", key);

        return id;
    }
}

    

