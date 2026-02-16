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
#include <Engine/Core/Log.h>

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

    // Put near your importer (same .cpp) or in GLTFImporterUtils.cpp

    static std::string GLTF_ComponentTypeName(int ct)
    {
        switch (ct)
        {
        case TINYGLTF_COMPONENT_TYPE_BYTE:           return "BYTE";
        case TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE:  return "UBYTE";
        case TINYGLTF_COMPONENT_TYPE_SHORT:          return "SHORT";
        case TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT: return "USHORT";
        case TINYGLTF_COMPONENT_TYPE_INT:            return "INT";
        case TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT:   return "UINT";
        case TINYGLTF_COMPONENT_TYPE_FLOAT:          return "FLOAT";
        case TINYGLTF_COMPONENT_TYPE_DOUBLE:         return "DOUBLE";
        default: return "UNKNOWN";
        }
    }

    static std::string GLTF_TypeName(int type)
    {
        switch (type)
        {
        case TINYGLTF_TYPE_SCALAR: return "SCALAR";
        case TINYGLTF_TYPE_VEC2:   return "VEC2";
        case TINYGLTF_TYPE_VEC3:   return "VEC3";
        case TINYGLTF_TYPE_VEC4:   return "VEC4";
        case TINYGLTF_TYPE_MAT2:   return "MAT2";
        case TINYGLTF_TYPE_MAT3:   return "MAT3";
        case TINYGLTF_TYPE_MAT4:   return "MAT4";
        default: return "UNKNOWN";
        }
    }

    // Optional: checks if matrix is close to identity
    static bool Mat4IsNearIdentity(const glm::mat4& m, float eps = 1e-4f)
    {
        glm::mat4 I(1.0f);
        for (int r = 0; r < 4; ++r)
            for (int c = 0; c < 4; ++c)
                if (fabs(m[r][c] - I[r][c]) > eps) return false;
        return true;
    }

    static void LogAccessor(const tinygltf::Model& model, int accessorIndex, const char* label)
    {
        if (accessorIndex < 0 || accessorIndex >= (int)model.accessors.size())
        {
            EE_CORE_WARN("[GLTFDBG] {} accessor = {} (invalid index)", label, accessorIndex);
            return;
        }

        const tinygltf::Accessor& acc = model.accessors[accessorIndex];
        const tinygltf::BufferView& bv = model.bufferViews[acc.bufferView];

        int byteStride = 0;
        // tinygltf helper exists, but we can compute effective stride:
        // If bv.byteStride != 0 -> use it, else tightly packed
        // You already use AccessPtr to get stride; this is just for logging.
        byteStride = (bv.byteStride > 0) ? (int)bv.byteStride : 0;

        EE_CORE_INFO("[GLTFDBG] Accessor {} '{}': count={}, type={}, comp={}, normalized={}, bufView={}, byteOffset={}, bv.byteStride={}",
            accessorIndex, label,
            acc.count,
            GLTF_TypeName(acc.type),
            GLTF_ComponentTypeName(acc.componentType),
            acc.normalized ? 1 : 0,
            acc.bufferView,
            acc.byteOffset,
            byteStride);
    }

    static void LogSkin(const tinygltf::Model& model, int skinIndex)
    {
        if (skinIndex < 0 || skinIndex >= (int)model.skins.size())
        {
            EE_CORE_WARN("[GLTFDBG] skinIndex {} invalid (skins={})", skinIndex, model.skins.size());
            return;
        }

        const tinygltf::Skin& skin = model.skins[skinIndex];

        EE_CORE_INFO("[GLTFDBG] Skin[{}] name='{}' jointsCount={} skeletonNode={}",
            skinIndex, skin.name, skin.joints.size(), skin.skeleton);

        // Log first few joint node indices and names
        const int maxPrint = 12;
        const int toPrint = (int)std::min<size_t>(skin.joints.size(), (size_t)maxPrint);
        for (int i = 0; i < toPrint; ++i)
        {
            int jNode = skin.joints[i];
            std::string jName = (jNode >= 0 && jNode < (int)model.nodes.size()) ? model.nodes[jNode].name : "<bad node index>";
            EE_CORE_INFO("  [GLTFDBG] skin.joints[{}] = node {} ('{}')", i, jNode, jName);
        }

        if (skin.inverseBindMatrices >= 0)
        {
            EE_CORE_INFO("[GLTFDBG] Skin[{}] inverseBindMatrices accessor = {}", skinIndex, skin.inverseBindMatrices);
            LogAccessor(model, skin.inverseBindMatrices, "inverseBindMatrices");

            const tinygltf::Accessor& ibmAcc = model.accessors[skin.inverseBindMatrices];
            if ((size_t)ibmAcc.count != skin.joints.size())
            {
                EE_CORE_WARN("[GLTFDBG] IBM count mismatch: ibmAcc.count={} but jointsCount={}",
                    ibmAcc.count, skin.joints.size());
            }
            if (ibmAcc.type != TINYGLTF_TYPE_MAT4)
            {
                EE_CORE_WARN("[GLTFDBG] IBM accessor type is {}, expected MAT4", GLTF_TypeName(ibmAcc.type));
            }
        }
        else
        {
            EE_CORE_WARN("[GLTFDBG] Skin[{}] has NO inverseBindMatrices accessor", skinIndex);
        }
    }

    // Reads a few joints/weights from CPU buffers exactly like your importer does,
    // and prints ranges + max joint index found.
    // Call this inside your primitive import after you have baseJoints/baseWeights/stride* and vCount.
    static void LogJointsWeightsSample(
        const tinygltf::Accessor* accJoints, const unsigned char* baseJoints, size_t strideJoints,
        const tinygltf::Accessor* accWeights, const unsigned char* baseWeights, size_t strideWeights,
        size_t vCount,
        const char* primName)
    {
        if (!accJoints || !baseJoints)
        {
            EE_CORE_INFO("[GLTFDBG] {}: no JOINTS_0", primName);
            return;
        }

        uint32_t maxJoint = 0;
        float minWsum = 1e9f;
        float maxWsum = -1e9f;

        const size_t sampleN = std::min<size_t>(vCount, 64);

        for (size_t i = 0; i < sampleN; ++i)
        {
            const unsigned char* pj = baseJoints + i * strideJoints;

            glm::uvec4 j(0);
            if (accJoints->componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE)
            {
                const uint8_t* jb = reinterpret_cast<const uint8_t*>(pj);
                j = glm::uvec4(jb[0], jb[1], jb[2], jb[3]);
            }
            else
            {
                const uint16_t* js = reinterpret_cast<const uint16_t*>(pj);
                j = glm::uvec4(js[0], js[1], js[2], js[3]);
            }

            maxJoint = std::max(maxJoint, std::max(std::max(j.x, j.y), std::max(j.z, j.w)));

            float wsum = 0.0f;
            if (accWeights && baseWeights)
            {
                const unsigned char* pw = baseWeights + i * strideWeights;
                glm::vec4 w(0.0f);

                if (accWeights->componentType == TINYGLTF_COMPONENT_TYPE_FLOAT)
                {
                    const float* wf = reinterpret_cast<const float*>(pw);
                    w = glm::vec4(wf[0], wf[1], wf[2], wf[3]);
                }
                else if (accWeights->componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE)
                {
                    const uint8_t* wb = reinterpret_cast<const uint8_t*>(pw);
                    w = glm::vec4(wb[0], wb[1], wb[2], wb[3]) / 255.0f;
                }
                else
                {
                    const uint16_t* ws = reinterpret_cast<const uint16_t*>(pw);
                    w = glm::vec4(ws[0], ws[1], ws[2], ws[3]) / 65535.0f;
                }

                wsum = w.x + w.y + w.z + w.w;
                minWsum = std::min(minWsum, wsum);
                maxWsum = std::max(maxWsum, wsum);
            }
        }

        EE_CORE_INFO("[GLTFDBG] {}: JOINTS_0 comp={} stride={} sampleN={} maxJointIndex={}",
            primName, GLTF_ComponentTypeName(accJoints->componentType), (uint32_t)strideJoints, (uint32_t)sampleN, maxJoint);

        if (accWeights && baseWeights)
        {
            EE_CORE_INFO("[GLTFDBG] {}: WEIGHTS_0 comp={} stride={} wSumRange=[{:.3f}..{:.3f}] (before normalize)",
                primName, GLTF_ComponentTypeName(accWeights->componentType), (uint32_t)strideWeights, minWsum, maxWsum);
        }
        else
        {
            EE_CORE_INFO("[GLTFDBG] {}: no WEIGHTS_0", primName);
        }
    }

    // Logs the important per-node info that commonly differs between assets.
    static void LogSkinnedNodeSummary1(
        const tinygltf::Model& model,
        int nodeIndex,
        const glm::mat4& nodeWorld)
    {
        const tinygltf::Node& node = model.nodes[nodeIndex];

        EE_CORE_INFO("[GLTFDBG] Node[{}] name='{}' mesh={} skin={} worldIsIdentity={}",
            nodeIndex,
            node.name,
            node.mesh,
            node.skin,
            Mat4IsNearIdentity(nodeWorld) ? 1 : 0);

        // Also log local TRS if available
        if (!node.translation.empty() || !node.rotation.empty() || !node.scale.empty())
        {
            glm::vec3 t(0.0f);
            glm::vec3 s(1.0f);
            glm::vec4 r(0.0f, 0.0f, 0.0f, 1.0f);

            if (node.translation.size() == 3) t = glm::vec3((float)node.translation[0], (float)node.translation[1], (float)node.translation[2]);
            if (node.scale.size() == 3)       s = glm::vec3((float)node.scale[0], (float)node.scale[1], (float)node.scale[2]);
            if (node.rotation.size() == 4)    r = glm::vec4((float)node.rotation[0], (float)node.rotation[1], (float)node.rotation[2], (float)node.rotation[3]);

            EE_CORE_INFO("  [GLTFDBG] TRS: T=({:.3f},{:.3f},{:.3f}) S=({:.3f},{:.3f},{:.3f}) Rquat=({:.3f},{:.3f},{:.3f},{:.3f})",
                t.x, t.y, t.z, s.x, s.y, s.z, r.x, r.y, r.z, r.w);
        }

        if (node.skin >= 0)
            LogSkin(model, node.skin);
    }

    static void LogAnimationSamplerInterpolations(const tinygltf::Model& model, const char* tag)
    {
        EE_CORE_INFO("[GLTFDBG] ----- Animation sampler interpolations: {} -----", tag);

        for (int ai = 0; ai < (int)model.animations.size(); ++ai)
        {
            const tinygltf::Animation& anim = model.animations[ai];

            int nLinear = 0, nStep = 0, nCubic = 0, nOther = 0;

            for (int si = 0; si < (int)anim.samplers.size(); ++si)
            {
                const tinygltf::AnimationSampler& s = anim.samplers[si];

                const std::string& interp = s.interpolation; // "LINEAR", "STEP", "CUBICSPLINE", or empty
                if (interp == "LINEAR" || interp.empty()) nLinear++;
                else if (interp == "STEP") nStep++;
                else if (interp == "CUBICSPLINE") nCubic++;
                else nOther++;

                EE_CORE_INFO("[GLTFDBG] anim[{}] '{}' sampler[{}] interp='{}' inputAcc={} outputAcc={}",
                    ai,
                    anim.name.empty() ? "<unnamed>" : anim.name,
                    si,
                    interp.empty() ? "<empty->LINEAR?>" : interp,
                    s.input,
                    s.output);
            }

            EE_CORE_INFO("[GLTFDBG] anim[{}] '{}' samplers: LINEAR={} STEP={} CUBICSPLINE={} OTHER={}",
                ai,
                anim.name.empty() ? "<unnamed>" : anim.name,
                nLinear, nStep, nCubic, nOther);
        }
    }




    GLTFImportResult GLTFImporter::Import(const std::string& path,MeshRegistry& meshReg, MaterialRegistry& matReg,
        SkeletonRegistry& skelReg, AnimationRegistry& animReg, const GLTFImportOptions& opts)
    {
        GLTFImportResult importResult{};
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
            importResult.report.ok = false;
            importResult.report.message = "tinygltf load failed: " + (err.empty() ? std::string("unknown") : err);
            return importResult;
        }

        EE_CORE_INFO("[GLTF] Images: {}, Textures: {}, Materials: {}",
            model.images.size(), model.textures.size(), model.materials.size());



        //LogAnimationSamplerInterpolations(model, path.c_str());

        uint32_t skeletonId = 0xFFFFFFFFu;
        std::vector<uint32_t> clipIds;

        if (!model.skins.empty())
        {
            skeletonId = GLTFIImporterUtils::LoadSkeletonFromModel(model, skelReg, path.c_str());
            importResult.skeletonId = skeletonId;
        }

        if (skeletonId != 0xFFFFFFFFu && !model.animations.empty()) 
        {
            GLTFIImporterUtils::LoadClipsFromModel(model, animReg, skeletonId, path.c_str(), clipIds);
            importResult.clipIds = clipIds;
        }



        // materials first
        importResult.materialIds.reserve(model.materials.size());
        for (const auto& m : model.materials)
        {
            uint32_t id = RegisterMaterial(model, m, opts, matReg);
            importResult.materialIds.push_back(id);

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
                    if (prim.material >= 0 && prim.material < (int)importResult.materialIds.size())
                    {
                        uint32_t matAsset = importResult.materialIds[prim.material];
                        if (matAsset != 0xFFFFFFFFu)
                        {
                            sub.materialDefaultId = matReg.ToRowId(matAsset);
                        }
                    }

                    sub.aabbMin = aabbMin;
                    sub.aabbMax = aabbMax;

                    // naming: use node name first
                    sub.name = nameBase + "_prim" + std::to_string((uint32_t)pi);
                    meshAsset.minL = glm::min(meshAsset.minL, aabbMin);
                    meshAsset.maxL = glm::max(meshAsset.maxL, aabbMax);
                    meshAsset.importScale = GLTFIImporterUtils::GuessImportScaleFromBounds(meshAsset.minL, meshAsset.maxL);
                    meshAsset.submeshes.push_back(std::move(sub));
                    //LogJointsWeightsSample(accJoints,baseJoints,strideJoints,accWeights,baseWeights,strideWeights,vCount, nameBase.c_str());

                  

                }
            }
        }
        EE_CORE_INFO("[GLTF] submeshes:");
        for (uint32_t i = 0; i < (uint32_t)meshAsset.submeshes.size(); ++i)
        {
            EE_CORE_INFO("  [{}] '{}'", i, meshAsset.submeshes[i].name);
        }
        bool hasAnimation = !importResult.clipIds.empty();
        bool hasSkeleton = (importResult.skeletonId != 0xFFFFFFFFu);
        bool hasMesh = !meshAsset.submeshes.empty();

        if (hasMesh)
        {

            // Mesh file with possible animations

            std::filesystem::path p(path);
            std::string meshName = p.stem().string();

            importResult.meshId = meshReg.RegisterMesh(meshName, meshAsset);
            importResult.report.ok = true;
            importResult.report.message = "Imported mesh (and animations): " + path;

            EE_CORE_INFO("[GLTF] final MeshAsset has {} submeshes", meshAsset.submeshes.size());
        }
        else if (hasAnimation || hasSkeleton)
        {
            // Animation-only or skeleton-only file  still valid!
            importResult.meshId = kInvalidMeshId; // no mesh
            importResult.report.ok = true;
            importResult.report.message = "Imported animation/skeleton only: " + path;
        }
        else 
        {
            // Nothing usable
            importResult.report.ok = false;
            importResult.report.message = "No mesh, skeleton, or animation found in: " + path;
        }
        return importResult;
    }

} // namespace Engine
