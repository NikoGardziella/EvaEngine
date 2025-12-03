#include "pch.h"
#include "GLTFImporter.h"
#include <cfloat>
#include <unordered_map>
#include <algorithm>
#include "glm/glm.hpp"
#include <Engine/Animation/3D/MeshRegistry.h>
#include <Engine/Animation/3D/MaterialRegistry.h>

#include "Engine/Animation/3D/Import/Utils/GLTFIImporterUtils.h"

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


    GLTFImportResult GLTFImporter::Import(const std::string& path,MeshRegistry& meshReg, MaterialRegistry& matReg,
        SkeletonRegistry& skelReg, AnimationRegistry& animReg, const GLTFImportOptions& opts)
    {
        GLTFImportResult R{};
        tinygltf::Model model;
        tinygltf::TinyGLTF loader;
        std::string err, warn;

        bool ok = false;
        if (path.size() >= 4)
        {
            const std::string ext = path.substr(path.size() - 4);
            if (ext == ".glb" || ext == ".GLB") ok = loader.LoadBinaryFromFile(&model, &err, &warn, path);
            else                                 ok = loader.LoadASCIIFromFile(&model, &err, &warn, path);
        }
        else 
        {
            ok = loader.LoadASCIIFromFile(&model, &err, &warn, path);
        }
        if (!warn.empty())
        {
            EE_CORE_WARN("no warnings");
        }
        if (!ok || !err.empty())
        {
            R.report.ok = false;
            R.report.message = "tinygltf load failed: " + (err.empty() ? std::string("unknown") : err);
            return R;
        }

        EE_CORE_INFO("[GLTF] Images: {}, Textures: {}, Materials: {}",
            model.images.size(), model.textures.size(), model.materials.size());





        uint32_t skeletonId = 0xFFFFFFFFu;
        std::vector<uint32_t> clipIds;

        if (!model.skins.empty())
        {
            skeletonId = GLTFIImporterUtils::LoadSkeletonFromModel(model, skelReg, path.c_str());
            R.skeletonId = skeletonId;
        }

        if (skeletonId != 0xFFFFFFFFu && !model.animations.empty()) 
        {
            GLTFIImporterUtils::LoadClipsFromModel(model, animReg, skeletonId, path.c_str(), clipIds);
            R.clipIds = clipIds;
        }



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
        for (const auto& mesh : model.meshes)
        {
            for (const auto& prim : mesh.primitives)
            {
                // topology
                const int mode = prim.mode < 0 ? TINYGLTF_MODE_TRIANGLES : prim.mode;
                if (mode != TINYGLTF_MODE_TRIANGLES)
                {
                    // TODO: triangulate or skip
                    continue;
                }

                auto itJ = prim.attributes.find("JOINTS_0");
                auto itW = prim.attributes.find("WEIGHTS_0");
                bool isSkinnedPrim = (itJ != prim.attributes.end() && itW != prim.attributes.end());

                if (isSkinnedPrim) {
                    meshAsset.isSkinned = true;
                    meshAsset.skeletonId = skeletonId;
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


                // JOINTS_0
                const tinygltf::Accessor* accJoints = nullptr;
                size_t strideJoints = 0;
                const unsigned char* baseJoints = nullptr;

                if (auto it = prim.attributes.find("JOINTS_0"); it != prim.attributes.end())
                {
                    accJoints = &model.accessors[it->second];
                    if (accJoints->type == TINYGLTF_TYPE_VEC4 &&
                        (accJoints->componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE ||
                            accJoints->componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT))
                    {
                        baseJoints = AccessPtr(model, *accJoints, strideJoints);
                    }
                    else
                    {
                        EE_CORE_WARN("[GLTF] JOINTS_0 has unsupported componentType={} type={}",
                            accJoints->componentType, accJoints->type);
                        accJoints = nullptr;
                    }
                }

                // WEIGHTS_0
                const tinygltf::Accessor* accWeights = nullptr;
                size_t strideWeights = 0;
                const unsigned char* baseWeights = nullptr;

                if (auto it = prim.attributes.find("WEIGHTS_0"); it != prim.attributes.end())
                {
                    accWeights = &model.accessors[it->second];
                    if (accWeights->type == TINYGLTF_TYPE_VEC4 &&
                        (accWeights->componentType == TINYGLTF_COMPONENT_TYPE_FLOAT ||
                            accWeights->componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE ||
                            accWeights->componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT))
                    {
                        baseWeights = AccessPtr(model, *accWeights, strideWeights);
                    }
                    else
                    {
                        EE_CORE_WARN("[GLTF] WEIGHTS_0 has unsupported componentType={} type={}",
                            accWeights->componentType, accWeights->type);
                        accWeights = nullptr;
                    }
                }

                // build vertex array in your pipeline layout
                std::vector<Vertex> verts(vCount);
                glm::vec3 aabbMin(FLT_MAX), aabbMax(-FLT_MAX);

                for (size_t i = 0; i < vCount; ++i)
                {
                    Vertex v{};
                    v.pos = ReadVec3(basePos + i * stridePos);

                    if (accNrm)
                        v.nrm = glm::normalize(ReadVec3(baseNrm + i * strideNrm));
                    else
                        v.nrm = glm::vec3(0, 1, 0);

                    if (accUv)
                    {
                        v.uv = ReadVec2(baseUv + i * strideUv);
                        if (opts.flipV) v.uv.y = 1.0f - v.uv.y;
                    }
                    else
                    {
                        v.uv = glm::vec2(0.0f);
                    }

                    // ---- JOINTS_0 ----
                    if (accJoints)
                    {
                        const unsigned char* p = baseJoints + i * strideJoints;
                        if (accJoints->componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE)
                        {
                            const uint8_t* jb = reinterpret_cast<const uint8_t*>(p);
                            v.joints = glm::uvec4(jb[0], jb[1], jb[2], jb[3]);
                        }
                        else // UNSIGNED_SHORT
                        {
                            const uint16_t* js = reinterpret_cast<const uint16_t*>(p);
                            v.joints = glm::uvec4(js[0], js[1], js[2], js[3]);
                        }
                    }
                    else
                    {
                        v.joints = glm::uvec4(0); // safe default
                    }

                    // ---- WEIGHTS_0 ----
                    if (accWeights)
                    {
                        const unsigned char* p = baseWeights + i * strideWeights;
                        glm::vec4 w;

                        if (accWeights->componentType == TINYGLTF_COMPONENT_TYPE_FLOAT)
                        {
                            const float* wf = reinterpret_cast<const float*>(p);
                            w = glm::vec4(wf[0], wf[1], wf[2], wf[3]);
                        }
                        else if (accWeights->componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE)
                        {
                            const uint8_t* wb = reinterpret_cast<const uint8_t*>(p);
                            w = glm::vec4(wb[0], wb[1], wb[2], wb[3]) / 255.0f;
                        }
                        else // UNSIGNED_SHORT
                        {
                            const uint16_t* ws = reinterpret_cast<const uint16_t*>(p);
                            w = glm::vec4(ws[0], ws[1], ws[2], ws[3]) / 65535.0f;
                        }

                        float sum = w.x + w.y + w.z + w.w;
                        if (sum > 0.0f)
                            v.weights = w / sum;
                        else
                            v.weights = glm::vec4(0.0, 0.0, 0.0, 0.0); // fallback
                    }
                    else
                    {
                        v.weights = glm::vec4(0.0, 0.0, 0.0, 0.0); // fallback
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


                uint32_t maxJoint = 0;
                float maxWeightErr = 0.0f;
                uint32_t zeroWeightVerts = 0;

                for (size_t i = 0; i < vCount; ++i)
                {
                    const auto& v = verts[i];

                    // joints
                    maxJoint = std::max(maxJoint,
                        std::max(std::max(v.joints.x, v.joints.y),
                            std::max(v.joints.z, v.joints.w)));

                    // weights
                    float wSum = v.weights.x + v.weights.y + v.weights.z + v.weights.w;
                    maxWeightErr = std::max(maxWeightErr, std::fabs(wSum - 1.0f));

                    if (wSum == 0.0f)
                        ++zeroWeightVerts;
                }

                const SkeletonAsset* sassetPtr = nullptr;
                if (meshAsset.isSkinned && skeletonId != 0xFFFFFFFFu)
                    sassetPtr = &skelReg.Get(skeletonId);

                EE_CORE_INFO("[GLTF] prim debug: vCount={} maxJoint={} zeroWeightVerts={} max|sum(w)-1|={} bones={}",
                    vCount, maxJoint, zeroWeightVerts, maxWeightErr,
                    sassetPtr ? (int)sassetPtr->parent.size() : -1);



                // indices
                std::vector<uint32_t> indices;
                if (prim.indices >= 0)
                {
                    const tinygltf::Accessor& accIdx = model.accessors[prim.indices];
                    ReadIndicesU32(model, accIdx, indices);
                }
                else
                {
                    indices.resize(vCount);
                    for (uint32_t i = 0; i < indices.size(); ++i) indices[i] = i;
                }

                if (!accNrm && opts.generateFlatNormalsIfMissing)
                {
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

        if (!meshAsset.submeshes.empty())
        {
            R.meshId = meshReg.Register(meshAsset);
            R.report.ok = true;
            R.report.message = "Imported: " + path;
        }
        else
        {
            R.report.ok = false;
            R.report.message = "No TRIANGLES primitives found in: " + path;
        }
        return R;
    }

} // namespace Engine
