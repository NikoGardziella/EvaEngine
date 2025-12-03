#include "pch.h"
#include "GLTFIImporterUtils.h"
#include "glm/glm.hpp"
#include <glm/gtc/quaternion.hpp>
#include "Engine/Animation/3D/AnimationRegistry.h"


namespace Engine {


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
        }

        // --------------------------------------------------------
        // Fill parent array: parent[bone] = parent bone index or -1
        // (walk up ancestors until we find another joint)
        // --------------------------------------------------------
        for (int bi = 0; bi < (int)boneCount; ++bi)
        {
            int nodeIndex = skin.joints[bi];

            // Start from the direct parent node
            int pNode = (nodeIndex >= 0 && nodeIndex < (int)nodeParent.size())
                ? nodeParent[nodeIndex]
                : -1;

            int parentBone = -1;

            // Walk up until we find an ancestor that is also a joint,
            // or we run out of parents.
            while (pNode >= 0)
            {
                auto it = nodeToBone.find(pNode);
                if (it != nodeToBone.end())
                {
                    parentBone = it->second;  // found a parent bone
                    break;
                }
                pNode = nodeParent[pNode];   // go up one level
            }

            asset.parent[bi] = (int16_t)parentBone;
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

        const SkeletonAsset& sasset = animReg.GetSkeletonRegistry().Get(skeletonId);
        const size_t boneCount = sasset.parent.size();
        if (boneCount == 0) {
            EE_CORE_WARN("[GLTF] {} skeleton {} has zero bones", debugName, skeletonId);
            return;
        }

        // Build node -> bone map from sasset.jointNodes
        std::unordered_map<int, int> nodeToBone;
        for (int bi = 0; bi < (int)boneCount; ++bi) {
            if (bi < (int)sasset.jointNodes.size()) {
                nodeToBone[sasset.jointNodes[bi]] = bi;
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
            float t) -> glm::vec3
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
                float t0 = times[idx0];
                float t1 = times[idx1];
                float alpha = (t1 > t0) ? (t - t0) / (t1 - t0) : 0.0f;
                return glm::mix(values[idx0], values[idx1], alpha);
            };

        auto sampleQuatCurve = [](const std::vector<float>& times,
            const std::vector<glm::quat>& values,
            float t) -> glm::quat
            {
                if (times.empty() || values.empty())
                    return glm::quat(1.0f, 0.0f, 0.0f, 0.0f);

                if (t <= times.front())
                    return values.front();
                if (t >= times.back())
                    return values.back();

                auto it = std::lower_bound(times.begin(), times.end(), t);
                size_t idx1 = (size_t)std::distance(times.begin(), it);
                size_t idx0 = idx1 - 1;
                float t0 = times[idx0];
                float t1 = times[idx1];
                float alpha = (t1 > t0) ? (t - t0) / (t1 - t0) : 0.0f;
                return glm::normalize(glm::slerp(values[idx0], values[idx1], alpha));
            };

        // For each animation in the file
        for (int ai = 0; ai < (int)model.animations.size(); ++ai) {
            const auto& anim = model.animations[ai];

            // Per-bone raw curves (separate times for T/R/S)
            struct TRSCurves {
                std::vector<float>      tTimes;
                std::vector<glm::vec3>  tValues;
                std::vector<float>      rTimes;
                std::vector<glm::quat>  rValues;
                std::vector<float>      sTimes;
                std::vector<glm::vec3>  sValues;
            };
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

                if (ch.target_path == "translation") {
                    readFloats(timeAcc, dst.tTimes);
                    const tinygltf::Accessor& valAcc = model.accessors[samp.output];
                    readVec3s(valAcc, dst.tValues);
                    if (!dst.tTimes.empty())
                        globalMaxTime = std::max(globalMaxTime, dst.tTimes.back());
                }
                else if (ch.target_path == "rotation") {
                    readFloats(timeAcc, dst.rTimes);
                    const tinygltf::Accessor& valAcc = model.accessors[samp.output];
                    readQuats(valAcc, dst.rValues);
                    if (!dst.rTimes.empty())
                        globalMaxTime = std::max(globalMaxTime, dst.rTimes.back());
                }
                else if (ch.target_path == "scale") {
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

                for (float t : merged) {
                    glm::vec3 T = c.tTimes.empty()
                        ? glm::vec3(0.0f)
                        : sampleVec3Curve(c.tTimes, c.tValues, t);
                    glm::vec3 S = c.sTimes.empty()
                        ? glm::vec3(1.0f)
                        : sampleVec3Curve(c.sTimes, c.sValues, t);
                    glm::quat R = c.rTimes.empty()
                        ? glm::quat(1.0f, 0.0f, 0.0f, 0.0f)
                        : sampleQuatCurve(c.rTimes, c.rValues, t);

                    chOut.times.push_back(t);
                    chOut.T.push_back(T);
                    chOut.R.push_back(R);
                    chOut.S.push_back(S);
                }
            }

            clip.duration = globalMaxTime;

            uint32_t clipId = animReg.Register(clip);
            outClipIds.push_back(clipId);

            EE_CORE_INFO("[GLTF] {} animation '{}' => duration {:.3f}s, id={}",
                debugName,
                clip.name.c_str(),
                clip.duration,
                clipId);
        }
    }


}