#pragma once
#include "glm/glm.hpp"
#include <vulkan/vulkan_core.h>

namespace Engine {

    struct SubmeshRange
    {
        // gpu handles/offsets if you track them here (optional)
        uint32_t firstIndex = 0;
        uint32_t indexCount = 0;
        uint32_t baseVertex = 0;
        uint32_t materialDefaultId = 0xFFFFFFFFu;
        glm::vec3 aabbMin{ 0 }, aabbMax{ 0 };
    };

    struct MeshAsset {
        // One pair of buffers for the whole mesh
        VkBuffer     vertexBuffer = VK_NULL_HANDLE;
        VkBuffer     indexBuffer = VK_NULL_HANDLE;
        VkDeviceSize vbOffset = 0;   // usually 0
        VkDeviceSize ibOffset = 0;   // usually 0
        uint32_t     vertexCount = 0;
        uint32_t     indexCount = 0;
        uint32_t     id;
        uint32_t     skeletonId;
        std::vector<SubmeshRange> submeshes;
        bool isSkinned = false;

        glm::vec3 minL = glm::vec3(0);
        glm::vec3 maxL = glm::vec3(0);
        
    };

    class MeshRegistry
    {

    public:
        MeshAsset& GetMesh(uint32_t meshId);

        MeshId FindMeshId(const std::string& key) const;
        const MeshAsset* GetMeshByKey(const std::string& key) const;

        const MeshAsset& GetMesh(uint32_t meshId) const;


        MeshId RegisterMesh(const std::string& key, const MeshAsset& asset);

      

    private:
        std::vector<MeshAsset> m_meshes;
        std::unordered_map<std::string, MeshId> m_keyToId;
    };

}
