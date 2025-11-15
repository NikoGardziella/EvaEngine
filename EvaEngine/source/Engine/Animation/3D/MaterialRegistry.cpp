#include "pch.h"
#include "MaterialRegistry.h"

namespace Engine {

    uint32_t MaterialRegistry::Register(const MaterialAsset& m) 
    {
        uint32_t id = (uint32_t)m_assets.size();
        MaterialAsset copy = m; copy.id = id;
        m_assets.push_back(copy);
        // assign row index if not already
        uint32_t row = (uint32_t)m_gpuTable.size();
        m_assetToRow.push_back(row);
        m_gpuTable.push_back(copy.gpu);
        m_dirty = true;
        return id;
    }

    const MaterialAsset& MaterialRegistry::Get(uint32_t id) const {
        return m_assets.at(id);
    }

    uint32_t MaterialRegistry::ToRowId(uint32_t materialAssetId) const {
        return m_assetToRow.at(materialAssetId);
    }

    void MaterialRegistry::EnsureOnGPU(VulkanCtx& vk) {
        if (!m_dirty || !vk.materialTableMappedPtr) return;
        std::memcpy(vk.materialTableMappedPtr, m_gpuTable.data(), m_gpuTable.size() * sizeof(MaterialGPU));
        m_dirty = false;
    }

}
