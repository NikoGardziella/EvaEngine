#pragma once
#include <string>
#include <vector>
#include <glm/glm.hpp>
#include <vulkan/vulkan_core.h>
#include <functional>
#include <Engine/Animation/3D/MeshRegistry.h>
#include <Engine/Animation/3D/MaterialRegistry.h>

namespace Engine {

    // pipeline vertex
    struct Vertex {
        glm::vec3 pos;
        glm::vec3 nrm;
        glm::vec2 uv;
    };





    // Import options/callbacks
    struct TextureSource {
        std::string debugName;
        bool sRGB = true;
        // extend with raw pixels later
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

    struct GLTFImportResult 
    {
        ImportReport report;
        uint32_t meshId = 0;
        std::vector<uint32_t> materialIds; // asset ids returned by MaterialRegistry::Register
    };

    class GLTFImporter 
    {
    public:
        // Load .glb or .gltf with tinygltf, create materials, upload primitives via callback, and register one MeshAsset
        GLTFImportResult Import(const std::string& path,
            MeshRegistry& meshReg,
            MaterialRegistry& matReg,
            const GLTFImportOptions& opts);
    };

} 