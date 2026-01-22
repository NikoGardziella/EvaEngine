#pragma once
#include <string>
#include <vector>

#include <vulkan/vulkan_core.h>
#include <functional>
#include <Engine/Animation/3D/MeshRegistry.h>
#include <Engine/Animation/3D/MaterialRegistry.h>
#include <Engine/Animation/3D/SkeletonRegistry.h>
#include <Engine/Animation/3D/AnimationRegistry.h>

namespace Engine {

    // pipeline vertex
    struct Vertex {
        glm::vec3 pos;   // location = 0  -> VK_FORMAT_R32G32B32_SFLOAT
        glm::vec3 nrm;   // location = 1  -> VK_FORMAT_R32G32B32_SFLOAT
        glm::vec2 uv;    // location = 2  -> VK_FORMAT_R32G32_SFLOAT
        glm::uvec4 joints; // location 3 (JOINTS_0)
        glm::vec4  weights;// location 4 (WEIGHTS_0)
    };





    // Import options/callbacks
    struct TextureSource {
        std::string debugName;
        bool        sRGB = false;
        int         width = 0;
        int         height = 0;
        int         channels = 0;
        const unsigned char* data = nullptr;
        size_t      dataSize = 0;
    };

    struct PrimitiveUpload
    {
        const Vertex* vertices = nullptr;
        size_t          vertexCount = 0;
        const uint32_t* indices = nullptr;
        size_t          indexCount = 0;

        glm::vec3 aabbMin{ 0 }, aabbMax{ 0 };
        bool hasNormals = false;
        bool hasUV0 = false;
    };

    struct GLTFImportOptions
    {
        std::function<SubmeshRange(const PrimitiveUpload&)> uploadPrimitive;

        std::function<uint32_t(const TextureSource&)> loadTexture;

        bool generateFlatNormalsIfMissing = true;
        bool flipV = false; // flip UV.y
    };

    struct ImportReport
    {
        bool ok = false;
        std::string message;
    };

    struct GLTFImportResult {
        bool ok = false;
        std::string message;
        ImportReport report;

        uint32_t meshId = 0xFFFFFFFFu;
        std::vector<uint32_t> materialIds;

        uint32_t skeletonId = 0xFFFFFFFFu;
        std::vector<uint32_t> clipIds; // all clips found in this file
    };

    class GLTFImporter 
    {
    public:
        
        GLTFImportResult Import(const std::string& path, MeshRegistry& meshReg, MaterialRegistry& matReg, SkeletonRegistry& skelReg, AnimationRegistry& animReg, const GLTFImportOptions& opts);
    };

} 