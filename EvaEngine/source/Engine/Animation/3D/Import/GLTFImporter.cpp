#include "pch.h"
#include "GLTFImporter.h"
#include <cfloat>
#include <unordered_map>
#include <algorithm>
#include "glm/glm.hpp"
#include <Engine/Animation/3D/MeshRegistry.h>
#include <Engine/Animation/3D/MaterialRegistry.h>



#include <tiny_gltf.h>

namespace Engine {

    static void GenFlatNormals(const uint32_t* idx, size_t idxCount,
        Vertex* vtx, size_t vtxCount)
    {
        (void)vtxCount; // not strictly needed
        // zero
        for (size_t i = 0; i < vtxCount; ++i) vtx[i].nrm = glm::vec3(0);

        for (size_t i = 0; i + 2 < idxCount; i += 3) {
            uint32_t i0 = idx[i + 0], i1 = idx[i + 1], i2 = idx[i + 2];
            glm::vec3 e1 = vtx[i1].pos - vtx[i0].pos;
            glm::vec3 e2 = vtx[i2].pos - vtx[i0].pos;
            glm::vec3 n = glm::normalize(glm::cross(e1, e2));
            vtx[i0].nrm += n; vtx[i1].nrm += n; vtx[i2].nrm += n;
        }
        for (size_t i = 0; i < vtxCount; ++i)
        {
            vtx[i].nrm = glm::length(vtx[i].nrm) > 0.0f ? glm::normalize(vtx[i].nrm) : glm::vec3(0, 1, 0);
        }
    }

    static glm::vec3 ReadVec3(const unsigned char* p) {
        const float* f = reinterpret_cast<const float*>(p);
        return glm::vec3(f[0], f[1], f[2]);
    }
    static glm::vec2 ReadVec2(const unsigned char* p) {
        const float* f = reinterpret_cast<const float*>(p);
        return glm::vec2(f[0], f[1]);
    }

    static const unsigned char* AccessPtr(const tinygltf::Model& model,
        const tinygltf::Accessor& acc,
        size_t& strideOut)
    {
        const auto& view = model.bufferViews[acc.bufferView];
        const auto& buf = model.buffers[view.buffer];
        size_t byteOffset = view.byteOffset + acc.byteOffset;
        strideOut = view.byteStride ? view.byteStride
            : tinygltf::GetComponentSizeInBytes(acc.componentType)
            * tinygltf::GetNumComponentsInType(acc.type);
        return buf.data.data() + byteOffset;
    }

    static void ReadIndicesU32(const tinygltf::Model& model,
        const tinygltf::Accessor& acc,
        std::vector<uint32_t>& out)
    {
        size_t stride = 0;
        const unsigned char* base = AccessPtr(model, acc, stride);
        out.resize(acc.count);
        switch (acc.componentType) {
        case TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT: {
            const uint16_t* p = reinterpret_cast<const uint16_t*>(base);
            for (size_t i = 0; i < acc.count; ++i) out[i] = p[i];
        } break;
        case TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE: {
            const uint8_t* p = reinterpret_cast<const uint8_t*>(base);
            for (size_t i = 0; i < acc.count; ++i) out[i] = p[i];
        } break;
        case TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT: {
            const uint32_t* p = reinterpret_cast<const uint32_t*>(base);
            for (size_t i = 0; i < acc.count; ++i) out[i] = p[i];
        } break;
        default: out.clear(); break;
        }
    }
    static uint32_t RegisterMaterial(const tinygltf::Model& model,
        const tinygltf::Material& m,
        const GLTFImportOptions& opts,
        MaterialRegistry& matReg)
    {
        MaterialGPU mgpu{};

        // ---- sane defaults ----
        mgpu.baseColorFactor = glm::vec4(1.0f); // GLTF default
        mgpu.metallicFactor = 1.0f;
        mgpu.roughnessFactor = 1.0f;

        mgpu.baseColorTex = 0xFFFFFFFFu;
        mgpu.normalTex = 0xFFFFFFFFu;
        mgpu.ormTex = 0xFFFFFFFFu;
        mgpu.emissiveTex = 0xFFFFFFFFu;

        mgpu.flags = 0;
        mgpu._pad = 0;

        // ---- override with GLTF values if present ----

        // baseColorFactor
        if (m.pbrMetallicRoughness.baseColorFactor.size() == 4)
        {
            mgpu.baseColorFactor = glm::vec4(
                static_cast<float>(m.pbrMetallicRoughness.baseColorFactor[0]),
                static_cast<float>(m.pbrMetallicRoughness.baseColorFactor[1]),
                static_cast<float>(m.pbrMetallicRoughness.baseColorFactor[2]),
                static_cast<float>(m.pbrMetallicRoughness.baseColorFactor[3]));
        }

        // metallic / roughness
        if (m.pbrMetallicRoughness.metallicFactor >= 0.0)
            mgpu.metallicFactor = static_cast<float>(m.pbrMetallicRoughness.metallicFactor);

        if (m.pbrMetallicRoughness.roughnessFactor >= 0.0)
            mgpu.roughnessFactor = static_cast<float>(m.pbrMetallicRoughness.roughnessFactor);

        // Helper to turn tinygltf texture index -> engine texture index
        auto loadTex = [&](int texIndex, bool sRGB) -> uint32_t
            {
                if (texIndex < 0)
                    return 0xFFFFFFFFu;

              
                const tinygltf::Texture& t = model.textures[texIndex];
                
                const tinygltf::Image& img = model.images[t.source];

                TextureSource ts{};
                ts.debugName = img.uri.empty()
                    ? ("gltf_embedded_tex_" + std::to_string(texIndex))
                    : img.uri;

                ts.sRGB = sRGB;
                ts.width = img.width;
                ts.height = img.height;
                ts.channels = img.component;
                ts.data = img.image.data();
                ts.dataSize = img.image.size();

                // Callback: returns engine-side texture handle/index
                if (opts.loadTexture)
                    return opts.loadTexture(ts);

                return 0xFFFFFFFFu;
            };

        // ---- bind textures to slots ----

        // baseColor / albedo
        mgpu.baseColorTex =
            loadTex(m.pbrMetallicRoughness.baseColorTexture.index, true);

        // metallic+roughness (ORM) packed texture, usually R=occlusion, G=roughness, B=metallic
        mgpu.ormTex =
            loadTex(m.pbrMetallicRoughness.metallicRoughnessTexture.index, false);

        // normal map
        mgpu.normalTex =
            loadTex(m.normalTexture.index, false);

        // emissive map
        mgpu.emissiveTex =
            loadTex(m.emissiveTexture.index, true);

        // ---- register into your material registry ----
        MaterialAsset asset{};
        asset.gpu = mgpu;

        uint32_t matAssetId = matReg.Register(asset);
        return matAssetId;
    }


    GLTFImportResult GLTFImporter::Import(const std::string& path,
        MeshRegistry& meshReg,
        MaterialRegistry& matReg,
        const GLTFImportOptions& opts)
    {
        GLTFImportResult R{};
        tinygltf::Model model;
        tinygltf::TinyGLTF loader;
        std::string err, warn;

        bool ok = false;
        if (path.size() >= 4) {
            const std::string ext = path.substr(path.size() - 4);
            if (ext == ".glb" || ext == ".GLB") ok = loader.LoadBinaryFromFile(&model, &err, &warn, path);
            else                                 ok = loader.LoadASCIIFromFile(&model, &err, &warn, path);
        }
        else {
            ok = loader.LoadASCIIFromFile(&model, &err, &warn, path);
        }
        if (!warn.empty()) { /* optional: log warn */ }
        if (!ok || !err.empty()) {
            R.report.ok = false;
            R.report.message = "tinygltf load failed: " + (err.empty() ? std::string("unknown") : err);
            return R;
        }

        EE_CORE_INFO("[GLTF] Images: {}, Textures: {}, Materials: {}",
            model.images.size(), model.textures.size(), model.materials.size());

        // materials first
        R.materialIds.reserve(model.materials.size());
        for (const auto& m : model.materials)
        {
            uint32_t id = RegisterMaterial(model, m, opts, matReg);
            R.materialIds.push_back(id);

            EE_CORE_INFO("[GLTF] Material: baseColorTexIdx = {}, ormTexIdx = {}, normalTexIdx = {}, emissiveTexIdx = {}",
                m.pbrMetallicRoughness.baseColorTexture.index,
                m.pbrMetallicRoughness.metallicRoughnessTexture.index,
                m.normalTexture.index,
                m.emissiveTexture.index);
        }

        MeshAsset meshAsset{};
        meshAsset.isSkinned = false; // we’re not handling skins here (yet)

        // iterate meshes / primitives
        for (const auto& mesh : model.meshes) {
            for (const auto& prim : mesh.primitives) {
                // topology
                const int mode = prim.mode < 0 ? TINYGLTF_MODE_TRIANGLES : prim.mode;
                if (mode != TINYGLTF_MODE_TRIANGLES) {
                    // TODO: triangulate or skip
                    continue;
                }

                // required: POSITION
                auto itPos = prim.attributes.find("POSITION");
                if (itPos == prim.attributes.end()) continue;
                const tinygltf::Accessor& accPos = model.accessors[itPos->second];
                if (accPos.type != TINYGLTF_TYPE_VEC3 || accPos.componentType != TINYGLTF_COMPONENT_TYPE_FLOAT) continue;

                size_t stridePos = 0;
                const unsigned char* basePos = AccessPtr(model, accPos, stridePos);
                const size_t vCount = accPos.count;

                // optional: NORMAL
                const tinygltf::Accessor* accNrm = nullptr;
                size_t strideNrm = 0; const unsigned char* baseNrm = nullptr;
                if (auto it = prim.attributes.find("NORMAL"); it != prim.attributes.end()) {
                    accNrm = &model.accessors[it->second];
                    if (accNrm->type == TINYGLTF_TYPE_VEC3 && accNrm->componentType == TINYGLTF_COMPONENT_TYPE_FLOAT)
                        baseNrm = AccessPtr(model, *accNrm, strideNrm);
                    else
                        accNrm = nullptr;
                }

                // TEXCOORD_0
                const tinygltf::Accessor* accUv = nullptr;
                size_t strideUv = 0; const unsigned char* baseUv = nullptr;
                if (auto it = prim.attributes.find("TEXCOORD_0"); it != prim.attributes.end())
                {
                    accUv = &model.accessors[it->second];
                    if (accUv->type == TINYGLTF_TYPE_VEC2 && accUv->componentType == TINYGLTF_COMPONENT_TYPE_FLOAT)
                        baseUv = AccessPtr(model, *accUv, strideUv);
                    else
                        accUv = nullptr;

                    EE_CORE_INFO("[GLTF] primitive hasUV0 = {}, vertices = {}", (accUv != nullptr), vCount);
                }


               
                // build vertex array in your pipeline layout
                std::vector<Vertex> verts(vCount);
                glm::vec3 aabbMin(FLT_MAX), aabbMax(-FLT_MAX);
                for (size_t i = 0; i < vCount; ++i)
                {
                    Vertex v{};
                    v.pos = ReadVec3(basePos + i * stridePos);
                    if (accNrm) v.nrm = glm::normalize(ReadVec3(baseNrm + i * strideNrm));
                    else        v.nrm = glm::vec3(0, 1, 0);
                    
                    if(accUv)
                    {
                        v.uv = ReadVec2(baseUv + i * strideUv);
                        if (opts.flipV) v.uv.y = 1.0f - v.uv.y;
                    }
                    else
                    {
                        v.uv = glm::vec2(0);
                    }
                    verts[i] = v;
                    aabbMin = glm::min(aabbMin, v.pos);
                    aabbMax = glm::max(aabbMax, v.pos);
                }

                glm::vec2 uvMin(FLT_MAX);
                glm::vec2 uvMax(-FLT_MAX);

                for (size_t i = 0; i < vCount; ++i)
                {
                    uvMin = glm::min(uvMin, verts[i].uv);
                    uvMax = glm::max(uvMax, verts[i].uv);
                }

                EE_CORE_INFO("[GLTF] UV range u:[{}..{}], v:[{}..{}]",
                    uvMin.x, uvMax.x, uvMin.y, uvMax.y);



                // indices
                std::vector<uint32_t> indices;
                if (prim.indices >= 0) {
                    const tinygltf::Accessor& accIdx = model.accessors[prim.indices];
                    ReadIndicesU32(model, accIdx, indices);
                }
                else {
                    indices.resize(vCount);
                    for (uint32_t i = 0; i < indices.size(); ++i) indices[i] = i;
                }

                if (!accNrm && opts.generateFlatNormalsIfMissing) {
                    GenFlatNormals(indices.data(), indices.size(), verts.data(), verts.size());
                }

                // hand to engine for VB/IB upload
                PrimitiveUpload up{};
                up.vertices = verts.data();
                up.vertexCount = verts.size();
                up.indices = indices.data();
                up.indexCount = indices.size();
                up.aabbMin = aabbMin;
                up.aabbMax = aabbMax;
                up.hasNormals = accNrm != nullptr;
                up.hasUV0 = accUv != nullptr;

                SubmeshRange sub{};
                if (opts.uploadPrimitive) sub = opts.uploadPrimitive(up);

                // default material (if present)
                if (prim.material >= 0 && prim.material < (int)R.materialIds.size()) {
                    uint32_t matAsset = R.materialIds[prim.material];
                    if (matAsset != 0xFFFFFFFFu)
                        sub.materialDefaultId = matReg.ToRowId(matAsset);
                }
                sub.aabbMin = aabbMin; sub.aabbMax = aabbMax;

                meshAsset.submeshes.push_back(sub);
            }
        }

        if (!meshAsset.submeshes.empty()) {
            R.meshId = meshReg.Register(meshAsset);
            R.report.ok = true;
            R.report.message = "Imported: " + path;
        }
        else {
            R.report.ok = false;
            R.report.message = "No TRIANGLES primitives found in: " + path;
        }
        return R;
    }

} // namespace Engine
