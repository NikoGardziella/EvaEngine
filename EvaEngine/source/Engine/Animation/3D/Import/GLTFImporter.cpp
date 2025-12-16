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
    static uint32_t RegisterMaterial(const tinygltf::Model& model, const tinygltf::Material& m,
        const GLTFImportOptions& opts, MaterialRegistry& matReg)
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
        mgpu.baseColorTex = loadTex(m.pbrMetallicRoughness.baseColorTexture.index, true);

        // metallic+roughness (ORM) packed texture, usually R=occlusion, G=roughness, B=metallic
        mgpu.ormTex = loadTex(m.pbrMetallicRoughness.metallicRoughnessTexture.index, false);

        // normal map
        mgpu.normalTex = loadTex(m.normalTexture.index, false);

        // emissive map
        mgpu.emissiveTex = loadTex(m.emissiveTexture.index, true);

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

      

        EE_CORE_INFO("[GLTF] model.meshes = {}", model.meshes.size());
        for (size_t mi = 0; mi < model.meshes.size(); ++mi)
        {
            const auto& m = model.meshes[mi];
            EE_CORE_INFO("[GLTF] mesh[{}] name='{}' has {} primitives",
                mi, m.name, m.primitives.size());

            for (size_t pi = 0; pi < m.primitives.size(); ++pi)
            {
                const auto& prim = m.primitives[pi];
                EE_CORE_INFO("    prim[{}]: material = {}", pi, prim.material);
            }
        }


        // ---- Rewrite of your mesh import loop ----
        {
            // Build a flat list of nodes with computed world transforms
            std::vector<int> nodeIndices;
            std::vector<glm::mat4> nodeWorlds;

            const int sceneIndex = (model.defaultScene >= 0) ? model.defaultScene : 0;
            if (sceneIndex >= 0 && sceneIndex < (int)model.scenes.size())
            {
                const tinygltf::Scene& sc = model.scenes[sceneIndex];
                for (int root : sc.nodes)
                    GLTFIImporterUtils::GLTF_GatherNodesDFS(model, root, glm::mat4(1.0f), nodeIndices, nodeWorlds);
            }
            else
            {
                // Fallback: treat all nodes as roots
                for (int ni = 0; ni < (int)model.nodes.size(); ++ni)
                    GLTFIImporterUtils::GLTF_GatherNodesDFS(model, ni, glm::mat4(1.0f), nodeIndices, nodeWorlds);
            }

            // Iterate nodes (not model.meshes), so we respect per-object transforms and names
            for (size_t nI = 0; nI < nodeIndices.size(); ++nI)
            {
                const int nodeIndex = nodeIndices[nI];
                const glm::mat4 nodeWorld = nodeWorlds[nI];

                const tinygltf::Node& node = model.nodes[nodeIndex];
                if (node.mesh < 0)
                    continue;

                const tinygltf::Mesh& mesh = model.meshes[node.mesh];

                // Normal matrix for this node
                const glm::mat3 N = glm::transpose(glm::inverse(glm::mat3(nodeWorld)));

                // Use node name for stable part naming (Blender object name)
                std::string nameBase;
                if (!node.name.empty()) nameBase = node.name;
                else if (!mesh.name.empty()) nameBase = mesh.name;
                else nameBase = "mesh" + std::to_string((uint32_t)node.mesh);

                for (size_t pi = 0; pi < mesh.primitives.size(); ++pi)
                {
                    const tinygltf::Primitive& prim = mesh.primitives[pi];

                    // topology
                    const int mode = prim.mode < 0 ? TINYGLTF_MODE_TRIANGLES : prim.mode;
                    if (mode != TINYGLTF_MODE_TRIANGLES)
                        continue;

                    // Skin detection
                    bool isSkinnedPrim = (prim.attributes.find("JOINTS_0") != prim.attributes.end()) &&
                        (prim.attributes.find("WEIGHTS_0") != prim.attributes.end());

                    if (isSkinnedPrim)
                    {
                        meshAsset.isSkinned = true;
                        meshAsset.skeletonId = skeletonId;
                    }

                    // required: POSITION
                    auto itPos = prim.attributes.find("POSITION");
                    if (itPos == prim.attributes.end()) continue;

                    const tinygltf::Accessor& accPos = model.accessors[itPos->second];
                    if (accPos.type != TINYGLTF_TYPE_VEC3 || accPos.componentType != TINYGLTF_COMPONENT_TYPE_FLOAT)
                        continue;

                    size_t stridePos = 0;
                    const unsigned char* basePos = AccessPtr(model, accPos, stridePos);
                    const size_t vCount = accPos.count;

                    // optional: NORMAL
                    const tinygltf::Accessor* accNrm = nullptr;
                    size_t strideNrm = 0;
                    const unsigned char* baseNrm = nullptr;
                    if (auto it = prim.attributes.find("NORMAL"); it != prim.attributes.end())
                    {
                        accNrm = &model.accessors[it->second];
                        if (accNrm->type == TINYGLTF_TYPE_VEC3 && accNrm->componentType == TINYGLTF_COMPONENT_TYPE_FLOAT)
                            baseNrm = AccessPtr(model, *accNrm, strideNrm);
                        else
                            accNrm = nullptr;
                    }

                    // TEXCOORD_0
                    const tinygltf::Accessor* accUv = nullptr;
                    size_t strideUv = 0;
                    const unsigned char* baseUv = nullptr;
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
                            EE_CORE_WARN("[GLTF] JOINTS_0 unsupported componentType={} type={}",
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
                            EE_CORE_WARN("[GLTF] WEIGHTS_0 unsupported componentType={} type={}",
                                accWeights->componentType, accWeights->type);
                            accWeights = nullptr;
                        }
                    }


                  
                    const glm::mat4 bakeM = isSkinnedPrim ? glm::mat4(1.0f) : nodeWorld;
                    const glm::mat3 bakeN = isSkinnedPrim ? glm::mat3(1.0f)
                        : glm::transpose(glm::inverse(glm::mat3(nodeWorld)));

                    // Build vertex array (apply nodeWorld transform)
                    std::vector<Vertex> verts(vCount);
                    glm::vec3 aabbMin(FLT_MAX), aabbMax(-FLT_MAX);

                    for (size_t i = 0; i < vCount; ++i)
                    {
                        Vertex v{};

                        glm::vec3 localPos = ReadVec3(basePos + i * stridePos);
                        v.pos = glm::vec3(bakeM * glm::vec4(localPos, 1.0f));

                        if (accNrm)
                        {
                            glm::vec3 localN = ReadVec3(baseNrm + i * strideNrm);
                            v.nrm = glm::normalize(bakeN * localN);
                        }
                        else
                        {
                            v.nrm = glm::vec3(0, 1, 0);
                        }

                        if (accUv)
                        {
                            v.uv = ReadVec2(baseUv + i * strideUv);
                            if (opts.flipV) v.uv.y = 1.0f - v.uv.y;
                        }
                        else
                        {
                            v.uv = glm::vec2(0.0f);
                        }

                        // JOINTS_0
                        if (accJoints)
                        {
                            const unsigned char* p = baseJoints + i * strideJoints;
                            if (accJoints->componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE)
                            {
                                const uint8_t* jb = reinterpret_cast<const uint8_t*>(p);
                                v.joints = glm::uvec4(jb[0], jb[1], jb[2], jb[3]);
                            }
                            else
                            {
                                const uint16_t* js = reinterpret_cast<const uint16_t*>(p);
                                v.joints = glm::uvec4(js[0], js[1], js[2], js[3]);
                            }
                        }
                        else
                        {
                            v.joints = glm::uvec4(0);
                        }

                        // WEIGHTS_0
                        if (accWeights)
                        {
                            const unsigned char* p = baseWeights + i * strideWeights;
                            glm::vec4 w(0.0f);

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
                            else
                            {
                                const uint16_t* ws = reinterpret_cast<const uint16_t*>(p);
                                w = glm::vec4(ws[0], ws[1], ws[2], ws[3]) / 65535.0f;
                            }

                            float sum = w.x + w.y + w.z + w.w;
                            if (sum > 0.0f) v.weights = w / sum;
                            else v.weights = glm::vec4(0.0f);
                        }
                        else
                        {
                            v.weights = glm::vec4(0.0f);
                        }

                        verts[i] = v;
                        aabbMin = glm::min(aabbMin, v.pos);
                        aabbMax = glm::max(aabbMax, v.pos);
                    }

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
                        for (uint32_t i = 0; i < (uint32_t)indices.size(); ++i) indices[i] = i;
                    }

                    if (!accNrm && opts.generateFlatNormalsIfMissing)
                        GenFlatNormals(indices.data(), indices.size(), verts.data(), verts.size());

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

                    // material
                    if (prim.material >= 0 && prim.material < (int)R.materialIds.size())
                    {
                        uint32_t matAsset = R.materialIds[prim.material];
                        if (matAsset != 0xFFFFFFFFu)
                            sub.materialDefaultId = matReg.ToRowId(matAsset);
                    }

                    sub.aabbMin = aabbMin;
                    sub.aabbMax = aabbMax;

                    // naming: use node name first
                    sub.name = nameBase + "_prim" + std::to_string((uint32_t)pi);
                    meshAsset.minL = glm::min(meshAsset.minL, aabbMin);
                    meshAsset.maxL = glm::max(meshAsset.maxL, aabbMax);
                    meshAsset.importScale = GLTFIImporterUtils::GuessImportScaleFromBounds(meshAsset.minL, meshAsset.maxL);
                    meshAsset.submeshes.push_back(std::move(sub));


                  

                }
            }
        }
        EE_CORE_INFO("[GLTF] submeshes:");
        for (uint32_t i = 0; i < (uint32_t)meshAsset.submeshes.size(); ++i)
        {
            EE_CORE_INFO("  [{}] '{}'", i, meshAsset.submeshes[i].name);
        }
        bool hasAnimation = !R.clipIds.empty();
        bool hasSkeleton = (R.skeletonId != 0xFFFFFFFFu);
        bool hasMesh = !meshAsset.submeshes.empty();

        if (hasMesh)
        {

            // Mesh file with possible animations

            std::filesystem::path p(path);
            std::string meshName = p.stem().string();

            R.meshId = meshReg.RegisterMesh(meshName, meshAsset);
            R.report.ok = true;
            R.report.message = "Imported mesh (and animations): " + path;

            EE_CORE_INFO("[GLTF] final MeshAsset has {} submeshes", meshAsset.submeshes.size());
        }
        else if (hasAnimation || hasSkeleton)
        {
            // Animation-only or skeleton-only file  still valid!
            R.meshId = kInvalidMeshId; // no mesh
            R.report.ok = true;
            R.report.message = "Imported animation/skeleton only: " + path;
        }
        else 
        {
            // Nothing usable
            R.report.ok = false;
            R.report.message = "No mesh, skeleton, or animation found in: " + path;
        }
        return R;
    }

} // namespace Engine
