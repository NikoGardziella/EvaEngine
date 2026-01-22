#include "pch.h"
#include "MeshRegistry.h"
#include <Engine/Core/Core.h>

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


    MeshId MeshRegistry::RegisterMesh(const std::string& key, MeshAsset& asset)
    {
        auto it = m_keyToId.find(key);
        if (it != m_keyToId.end())
            return it->second;

        MeshId id = (MeshId)m_meshes.size();
        asset.id = id;
        m_meshes.push_back(asset);
        m_keyToId[key] = id;

        return id;
    }

    uint32_t MeshRegistry::FindSubmeshContains(const MeshAsset& mesh, std::string_view needle)
    {
        for (uint32_t i = 0; i < (uint32_t)mesh.submeshes.size(); ++i)
            if (mesh.submeshes[i].name.find(needle) != std::string::npos)
                return i;
        return 0xFFFFFFFFu;
    }

}

    

