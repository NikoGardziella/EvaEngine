#include "pch.h"
#include "MeshRegistry.h"

namespace Engine {

    const MeshAsset& MeshRegistry::Get(uint32_t meshId) const 
    {
        return m_meshes.at(meshId);
    }

    MeshAsset& MeshRegistry::Get(uint32_t meshId)
    {
        return m_meshes.at(meshId);
    }

    uint32_t MeshRegistry::Register(const MeshAsset& m)
    {
        uint32_t id = (uint32_t)m_meshes.size();
        MeshAsset copy = m;
        copy.id = id;
        m_meshes.push_back(std::move(copy));
        return id;
    }

}

    

