#pragma once
#include "glm/glm.hpp"
#include <Engine/Renderer/3D/VulkanRenderer3D.h>
#include <vector>
#include <cstdint>


namespace Engine {

   
    static constexpr uint32_t kInvalidMeshId = 0xFFFFFFFFu;

    
    struct MaterialGPU {
        uint32_t baseColorTex;
        uint32_t normalTex;
        uint32_t  ormTex;
        uint32_t  emissiveTex;
        glm::vec4 baseColorFactor;
        float metallicFactor;
        float roughnessFactor;
        uint32_t flags;
        uint32_t _pad;
    };
    

    struct MaterialAsset {
        uint32_t id = {};
        MaterialGPU gpu{};
    };

    struct VulkanCtx {
        void* materialTableMappedPtr = nullptr; // set each frame by your renderer
        void* paletteMappedPtr = nullptr;
    };

    class MaterialRegistry {
    public:
        uint32_t Register(const MaterialAsset& m);
        const MaterialAsset& Get(uint32_t id) const;
        uint32_t ToRowId(uint32_t materialAssetId) const;

        void EnsureOnGPU(VulkanCtx& vk);
        const std::vector<MaterialAsset>& GetMaterials() const { return m_assets;  }
        const std::vector<MaterialGPU>& GPUTable() const { return m_gpuTable;  }
    private:
        bool m_dirty = false;
        std::vector<MaterialAsset> m_assets;
        std::vector<MaterialGPU>   m_gpuTable;
        std::vector<uint32_t>      m_assetToRow; // assetId -> row
    };

}
