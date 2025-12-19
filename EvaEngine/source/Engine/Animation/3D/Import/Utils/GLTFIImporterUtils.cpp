#include "pch.h"
#include "GLTFIImporterUtils.h"
#include "glm/glm.hpp"
#include <glm/gtc/quaternion.hpp>
#include "Engine/Animation/3D/AnimationRegistry.h"
#include <glm/gtc/type_ptr.hpp>


namespace Engine {


    float GLTFIImporterUtils::GuessImportScaleFromBounds(const glm::vec3& minL, const glm::vec3& maxL)
    {
        const float height = maxL.y - minL.y;

        // If it’s clearly in centimeters (Mixamo-ish), convert cm -> m
        // Typical Mixamo height might be 160..200 (cm) -> 1.6..2.0 m
        if (height > 10.0f && height < 400.0f)
            return 0.01f;

        // If its insanely large, bring it down to  size as fallback
        if (height >= 400.0f)
            return 1.7f / height;

        return 1.0f;
    }




    glm::mat4 GLTFIImporterUtils::GLTF_NodeLocalMatrix(const tinygltf::Node& n)
    {
        // If n.matrix is provided, prefer it
        if (n.matrix.size() == 16)
        {
            glm::mat4 M(1.0f);
            // glTF stores column-major
            const double* m = n.matrix.data();
            M[0][0] = (float)m[0];  M[1][0] = (float)m[1];  M[2][0] = (float)m[2];  M[3][0] = (float)m[3];
            M[0][1] = (float)m[4];  M[1][1] = (float)m[5];  M[2][1] = (float)m[6];  M[3][1] = (float)m[7];
            M[0][2] = (float)m[8];  M[1][2] = (float)m[9];  M[2][2] = (float)m[10]; M[3][2] = (float)m[11];
            M[0][3] = (float)m[12]; M[1][3] = (float)m[13]; M[2][3] = (float)m[14]; M[3][3] = (float)m[15];
            return M;
        }

        glm::vec3 T(0.0f);
        glm::vec3 S(1.0f);
        glm::quat R(1.0f, 0.0f, 0.0f, 0.0f); // w,x,y,z

        if (n.translation.size() == 3)
            T = glm::vec3((float)n.translation[0], (float)n.translation[1], (float)n.translation[2]);

        if (n.scale.size() == 3)
            S = glm::vec3((float)n.scale[0], (float)n.scale[1], (float)n.scale[2]);

        if (n.rotation.size() == 4)
        {
            // glTF rotation is [x,y,z,w]
            R = glm::quat((float)n.rotation[3], (float)n.rotation[0], (float)n.rotation[1], (float)n.rotation[2]);
        }

        glm::mat4 M(1.0f);
        M = glm::translate(glm::mat4(1.0f), T) * glm::mat4_cast(R) * glm::scale(glm::mat4(1.0f), S);
        return M;
    }


    void GLTFIImporterUtils::GLTF_GatherNodesDFS(const tinygltf::Model& model, int nodeIndex,
        const glm::mat4& parentWorld, std::vector<int>& outNodeIndices, std::vector<glm::mat4>& outNodeWorlds)
    {
        const tinygltf::Node& n = model.nodes[nodeIndex];
        glm::mat4 world = parentWorld * GLTF_NodeLocalMatrix(n);

        outNodeIndices.push_back(nodeIndex);
        outNodeWorlds.push_back(world);

        for (int child : n.children)
        {
            GLTF_GatherNodesDFS(model, child, world, outNodeIndices, outNodeWorlds);
        }
    }



    uint32_t GLTFIImporterUtils::LoadSkeletonFromModel(const tinygltf::Model& model,
        SkeletonRegistry& skelReg, const char* debugName)
    {
        if (model.skins.empty())
        {
            EE_CORE_ERROR("[GLTF] {} has no skins", debugName);
            return 0xFFFFFFFFu;
        }

        const tinygltf::Skin& skin = model.skins[0];

        SkeletonAsset asset{};

        const size_t boneCount = skin.joints.size();
        if (boneCount == 0)
        {
            EE_CORE_ERROR("[GLTF] {} skin has zero joints", debugName);
            return 0xFFFFFFFFu;
        }

        asset.parent.assign(boneCount, -1);
        asset.invBind.resize(boneCount, glm::mat4(1.0f));
        asset.restLocal.resize(boneCount, glm::mat4(1.0f));
        asset.jointNodes.resize(boneCount);
        asset.boneNames.resize(boneCount);    // <-- per bone, not per node

        const size_t nodeCount = model.nodes.size();

        // --------------------------------------------------------
        // Build nodeParent: glTF node index -> parent node index
        // --------------------------------------------------------
        std::vector<int> nodeParent(nodeCount, -1);
        for (int ni = 0; ni < (int)nodeCount; ++ni)
        {
            const tinygltf::Node& node = model.nodes[ni];
            for (int child : node.children)
            {
                if (child >= 0 && child < (int)nodeCount)
                    nodeParent[child] = ni;
            }
        }

        // --------------------------------------------------------
        // Map node index -> bone index for this skin
        // --------------------------------------------------------
        std::unordered_map<int, int> nodeToBone;
        nodeToBone.reserve(boneCount);

        for (int bi = 0; bi < (int)boneCount; ++bi)
        {
            int nodeIndex = skin.joints[bi];
            nodeToBone[nodeIndex] = bi;
            asset.jointNodes[bi] = nodeIndex;

            // boneNames[bi] from that node
            if (nodeIndex >= 0 && nodeIndex < (int)nodeCount)
            {
                const tinygltf::Node& node = model.nodes[nodeIndex];
                if (!node.name.empty())
                    asset.boneNames[bi] = node.name;
                else
                    asset.boneNames[bi] = fmt::format("Bone{}", bi);
            }
            else
            {
                asset.boneNames[bi] = fmt::format("Bone{}", bi);
            }
        }

        // --------------------------------------------------------
        // Fill parent array: parent[bone] = parent bone index or -1
        // --------------------------------------------------------
        for (int bi = 0; bi < (int)boneCount; ++bi)
        {
            int nodeIndex = skin.joints[bi];

            int pNode = (nodeIndex >= 0 && nodeIndex < (int)nodeParent.size())
                ? nodeParent[nodeIndex]
                : -1;

            int parentBone = -1;

            while (pNode >= 0)
            {
                auto it = nodeToBone.find(pNode);
                if (it != nodeToBone.end())
                {
                    parentBone = it->second;
                    break;
                }
                pNode = nodeParent[pNode];
            }

            asset.parent[bi] = (int16_t)parentBone;  // -1 for root
        }

        // --------------------------------------------------------
        // Read inverse bind matrices (if present)
        // --------------------------------------------------------
        if (skin.inverseBindMatrices >= 0)
        {
            const tinygltf::Accessor& acc = model.accessors[skin.inverseBindMatrices];
            const tinygltf::BufferView& bv = model.bufferViews[acc.bufferView];
            const tinygltf::Buffer& buf = model.buffers[bv.buffer];

            const uint8_t* base = buf.data.data() + bv.byteOffset + acc.byteOffset;
            const size_t   stride = acc.ByteStride(bv);

            for (size_t i = 0; i < boneCount; ++i)
            {
                const float* m = reinterpret_cast<const float*>(base + i * stride);
                glm::mat4 mat(
                    m[0], m[1], m[2], m[3],
                    m[4], m[5], m[6], m[7],
                    m[8], m[9], m[10], m[11],
                    m[12], m[13], m[14], m[15]
                );
                asset.invBind[i] = mat;
            }
        }
        else
        {
            EE_CORE_WARN("[GLTF] {} skin has no inverseBindMatrices; using identity", debugName);
            for (size_t i = 0; i < boneCount; ++i)
                asset.invBind[i] = glm::mat4(1.0f);
        }

        // --------------------------------------------------------
        // Compute restLocal (local TRS matrix) per bone from node
        // --------------------------------------------------------
        auto NodeLocalMatrix = [&](int nodeIndex) -> glm::mat4
            {
                if (nodeIndex < 0 || nodeIndex >= (int)model.nodes.size())
                    return glm::mat4(1.0f);

                const tinygltf::Node& node = model.nodes[nodeIndex];

                glm::vec3 T(0.0f);
                glm::vec3 S(1.0f);
                glm::quat R(1.0f, 0.0f, 0.0f, 0.0f);

                if (!node.translation.empty())
                    T = glm::vec3(
                        (float)node.translation[0],
                        (float)node.translation[1],
                        (float)node.translation[2]);

                if (!node.scale.empty())
                    S = glm::vec3(
                        (float)node.scale[0],
                        (float)node.scale[1],
                        (float)node.scale[2]);

                if (!node.rotation.empty())
                    R = glm::quat(
                        (float)node.rotation[3], // w
                        (float)node.rotation[0], // x
                        (float)node.rotation[1], // y
                        (float)node.rotation[2]  // z
                    );

                glm::mat4 M = glm::translate(glm::mat4(1.0f), T)
                    * glm::mat4_cast(R)
                    * glm::scale(glm::mat4(1.0f), S);

                return M;
            };

        for (int bi = 0; bi < (int)boneCount; ++bi)
        {
            int nodeIndex = skin.joints[bi];
            asset.restLocal[bi] = NodeLocalMatrix(nodeIndex);
        }

        // --------------------------------------------------------
        // Register skeleton
        // --------------------------------------------------------
        uint32_t id = skelReg.Register(asset);
        EE_CORE_INFO("[GLTF] {} skeleton => {} bones, id={}", debugName, boneCount, id);
        return id;
    }


    enum class InterpMode : uint8_t { Linear, Step };

    static InterpMode ToInterp(const std::string& s)
    {
        if (s == "STEP") return InterpMode::Step;
        return InterpMode::Linear; // empty or "LINEAR" treated as LINEAR
    }

    struct TRSCurves {
        std::vector<float>      tTimes;
        std::vector<glm::vec3>  tValues;
        std::vector<float>      rTimes;
        std::vector<glm::quat>  rValues;
        std::vector<float>      sTimes;
        std::vector<glm::vec3>  sValues;

        InterpMode tInterp = InterpMode::Linear;
        InterpMode rInterp = InterpMode::Linear;
        InterpMode sInterp = InterpMode::Linear;
    };

    static void DecomposeTRS(const glm::mat4& M, glm::vec3& T, glm::quat& R, glm::vec3& S)
    {
        // Translation
        T = glm::vec3(M[3]);

        // Columns (GLM column-major)
        glm::vec3 c0 = glm::vec3(M[0]);
        glm::vec3 c1 = glm::vec3(M[1]);
        glm::vec3 c2 = glm::vec3(M[2]);

        S.x = glm::length(c0);
        S.y = glm::length(c1);
        S.z = glm::length(c2);

        if (S.x > 0.0f) c0 /= S.x;
        if (S.y > 0.0f) c1 /= S.y;
        if (S.z > 0.0f) c2 /= S.z;

        glm::mat3 Rm(c0, c1, c2);
        R = glm::normalize(glm::quat_cast(Rm));
    }


    void GLTFIImporterUtils::LoadClipsFromModel(
        const tinygltf::Model& model,
        AnimationRegistry& animReg,
        uint32_t skeletonId,
        const char* debugName,
        std::vector<uint32_t>& outClipIds)
    {
        if (model.animations.empty()) {
            EE_CORE_WARN("[GLTF] {} has no animations", debugName);
            return;
        }

        const SkeletonAsset& skeletonAsset = animReg.GetSkeletonRegistry().Get(skeletonId);
        const size_t boneCount = skeletonAsset.parent.size();
        if (boneCount == 0) {
            EE_CORE_WARN("[GLTF] {} skeleton {} has zero bones", debugName, skeletonId);
            return;
        }

        // Build node -> bone map from sasset.jointNodes
        std::unordered_map<int, int> nodeToBone;
        for (int bi = 0; bi < (int)boneCount; ++bi) {
            if (bi < (int)skeletonAsset.jointNodes.size()) {
                nodeToBone[skeletonAsset.jointNodes[bi]] = bi;
            }
        }

        // Helpers to read data from glTF
        auto readFloats = [&](const tinygltf::Accessor& acc, std::vector<float>& out) {
            const tinygltf::BufferView& bv = model.bufferViews[acc.bufferView];
            const tinygltf::Buffer& buf = model.buffers[bv.buffer];
            const uint8_t* base = buf.data.data() + bv.byteOffset + acc.byteOffset;
            size_t stride = acc.ByteStride(bv);

            out.resize(acc.count);
            for (size_t i = 0; i < acc.count; ++i) {
                const float* v = reinterpret_cast<const float*>(base + i * stride);
                out[i] = *v;
            }
            };

        auto readVec3s = [&](const tinygltf::Accessor& acc, std::vector<glm::vec3>& out) {
            const tinygltf::BufferView& bv = model.bufferViews[acc.bufferView];
            const tinygltf::Buffer& buf = model.buffers[bv.buffer];
            const uint8_t* base = buf.data.data() + bv.byteOffset + acc.byteOffset;
            size_t stride = acc.ByteStride(bv);

            out.resize(acc.count);
            for (size_t i = 0; i < acc.count; ++i) {
                const float* v = reinterpret_cast<const float*>(base + i * stride);
                out[i] = glm::vec3(v[0], v[1], v[2]);
            }
            };

        auto readQuats = [&](const tinygltf::Accessor& acc, std::vector<glm::quat>& out) {
            const tinygltf::BufferView& bv = model.bufferViews[acc.bufferView];
            const tinygltf::Buffer& buf = model.buffers[bv.buffer];
            const uint8_t* base = buf.data.data() + bv.byteOffset + acc.byteOffset;
            size_t stride = acc.ByteStride(bv);

            out.resize(acc.count);
            for (size_t i = 0; i < acc.count; ++i) {
                const float* v = reinterpret_cast<const float*>(base + i * stride);
                // glTF quats are (x, y, z, w)
                out[i] = glm::quat(v[3], v[0], v[1], v[2]);
            }
            };
        auto sampleVec3Curve = [](const std::vector<float>& times,
            const std::vector<glm::vec3>& values,
            float t,
            InterpMode mode) -> glm::vec3
            {
                if (times.empty() || values.empty())
                    return glm::vec3(0.0f);

                if (t <= times.front())
                    return values.front();
                if (t >= times.back())
                    return values.back();

                auto it = std::lower_bound(times.begin(), times.end(), t);
                size_t idx1 = (size_t)std::distance(times.begin(), it);
                size_t idx0 = idx1 - 1;

                if (mode == InterpMode::Step)
                    return values[idx0]; // hold previous key

                float t0 = times[idx0];
                float t1 = times[idx1];
                float alpha = (t1 > t0) ? (t - t0) / (t1 - t0) : 0.0f;
                return glm::mix(values[idx0], values[idx1], alpha);
            };

        auto sampleQuatCurve = [](const std::vector<float>& times,
            const std::vector<glm::quat>& values,
            float t,
            InterpMode mode) -> glm::quat
            {
                if (times.empty() || values.empty())
                    return glm::quat(1.0f, 0.0f, 0.0f, 0.0f);

                glm::quat q0 = glm::normalize(values.front());
                glm::quat qN = glm::normalize(values.back());

                if (t <= times.front())
                    return q0;
                if (t >= times.back())
                    return qN;

                auto it = std::lower_bound(times.begin(), times.end(), t);
                size_t idx1 = (size_t)std::distance(times.begin(), it);
                size_t idx0 = idx1 - 1;

                glm::quat a = glm::normalize(values[idx0]);
                if (mode == InterpMode::Step)
                    return a; // hold previous key

                glm::quat b = glm::normalize(values[idx1]);

                // shortest-path
                if (glm::dot(a, b) < 0.0f) b = -b;

                float t0 = times[idx0];
                float t1 = times[idx1];
                float alpha = (t1 > t0) ? (t - t0) / (t1 - t0) : 0.0f;

                return glm::normalize(glm::slerp(a, b, alpha));
            };


        // For each animation in the file
        for (int ai = 0; ai < (int)model.animations.size(); ++ai) {
            const auto& anim = model.animations[ai];

            // Per-bone raw curves (separate times for T/R/S)
        
            std::vector<TRSCurves> curves(boneCount);

            float globalMaxTime = 0.0f;

            // Read all channels in this animation
            for (const auto& ch : anim.channels) {
                int nodeIndex = ch.target_node;
                auto itBone = nodeToBone.find(nodeIndex);
                if (itBone == nodeToBone.end())
                    continue; // not part of the skeleton

                int bone = itBone->second;
                if (bone < 0 || bone >= (int)boneCount)
                    continue;

                TRSCurves& dst = curves[bone];

                const tinygltf::AnimationSampler& samp = anim.samplers[ch.sampler];
                const tinygltf::Accessor& timeAcc = model.accessors[samp.input];
                const InterpMode interp = ToInterp(samp.interpolation);

                if (ch.target_path == "translation")
                {
                    dst.rInterp = interp;
                    readFloats(timeAcc, dst.tTimes);
                    const tinygltf::Accessor& valAcc = model.accessors[samp.output];
                    readVec3s(valAcc, dst.tValues);
                    if (!dst.tTimes.empty())
                        globalMaxTime = std::max(globalMaxTime, dst.tTimes.back());
                }
                else if (ch.target_path == "rotation")
                {
                    dst.rInterp = interp;
                    readFloats(timeAcc, dst.rTimes);
                    const tinygltf::Accessor& valAcc = model.accessors[samp.output];
                    readQuats(valAcc, dst.rValues);
                    if (!dst.rTimes.empty())
                        globalMaxTime = std::max(globalMaxTime, dst.rTimes.back());
                }
                else if (ch.target_path == "scale")
                {
                    dst.sInterp = interp;
                    readFloats(timeAcc, dst.sTimes);
                    const tinygltf::Accessor& valAcc = model.accessors[samp.output];
                    readVec3s(valAcc, dst.sValues);
                    if (!dst.sTimes.empty())
                        globalMaxTime = std::max(globalMaxTime, dst.sTimes.back());
                }
                else {
                    // weights / morph target animations ignored for now
                }
            }

            // Build final AnimationClip
            AnimationClip clip{};
            clip.name = anim.name;
            clip.channels.resize(boneCount);
            clip.skeletonId = skeletonId;
            for (uint32_t b = 0; b < boneCount; ++b) {
                clip.channels[b].bone = (uint16_t)b;

                TRSCurves& c = curves[b];

                // If bone has no keys at all, keep channel empty (SampleChannel should fallback to rest pose)
                if (c.tTimes.empty() && c.rTimes.empty() && c.sTimes.empty())
                    continue;

                // Merge all time keys for this bone
                std::vector<float> merged;
                merged.reserve(c.tTimes.size() + c.rTimes.size() + c.sTimes.size());

                merged.insert(merged.end(), c.tTimes.begin(), c.tTimes.end());
                merged.insert(merged.end(), c.rTimes.begin(), c.rTimes.end());
                merged.insert(merged.end(), c.sTimes.begin(), c.sTimes.end());

                std::sort(merged.begin(), merged.end());
                merged.erase(std::unique(merged.begin(), merged.end(),
                    [](float a, float b) { return std::fabs(a - b) < 1e-5f; }),
                    merged.end());

                auto& chOut = clip.channels[b];
                chOut.times.reserve(merged.size());
                chOut.T.reserve(merged.size());
                chOut.R.reserve(merged.size());
                chOut.S.reserve(merged.size());
                glm::vec3 restT; glm::quat restR; glm::vec3 restS;
                DecomposeTRS(skeletonAsset.restLocal[b], restT, restR, restS);

                for (float t : merged)
                {
                    glm::vec3 T = c.tTimes.empty()
                        ? restT
                        : sampleVec3Curve(c.tTimes, c.tValues, t, c.tInterp);

                    glm::vec3 S = c.sTimes.empty()
                        ? restS
                        : sampleVec3Curve(c.sTimes, c.sValues, t, c.sInterp);

                    glm::quat R = c.rTimes.empty()
                        ? restR
                        : sampleQuatCurve(c.rTimes, c.rValues, t, c.rInterp);

                    chOut.times.push_back(t);
                    chOut.T.push_back(T);
                    chOut.R.push_back(R);
                    chOut.S.push_back(S);
                }

            }

            clip.duration = globalMaxTime;

            uint32_t clipId = animReg.RegisterAnimation(clip.name, skeletonId ,clip);
            outClipIds.push_back(clipId);

            EE_CORE_INFO("[GLTF] {} animation '{}' => duration {:.3f}s, id={}",
                debugName,
                clip.name.c_str(),
                clip.duration,
                clipId);
        }
    }





}