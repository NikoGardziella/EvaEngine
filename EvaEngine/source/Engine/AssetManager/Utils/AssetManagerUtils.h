#pragma once
#include <Engine/Renderer/3D/VulkanRenderer3D.h>
#include <vector>
#include <Engine/Animation/3D/Import/GLTFImporter.h>
#include <glm/fwd.hpp>
#include <limits>
#include <Engine/Platform/Vulkan/VulkanTexture.h>
#include <Engine/Core/Core.h>
#include <Engine/Core/Log.h>
#include <Engine/Animation/3D/SkeletonRegistry.h>

namespace Engine {

	class AssetManagerUtils
	{

	public:

        struct GLTFAggregator {
            std::vector<Vertex>   allVerts;
            std::vector<uint32_t> allIdx;
        };

        struct DefaultTextureLoader {
            uint32_t operator()(const TextureSource& textureSource) const 
            {
                Ref<VulkanTexture> tex = std::make_shared<VulkanTexture>(textureSource);

                // 2) Register into VulkanRenderer3D texture array
                uint32_t index = VulkanRenderer3D::RegisterAlbedoTexture(tex);

                EE_CORE_INFO("[GLTF] Created texture '{}' -> array index {}", textureSource.debugName, index);

                return index;
            }
        };

        // Appends a primitive into the big per-mesh arrays
        struct PrimitiveAppender {
            GLTFAggregator* agg = nullptr; // must be valid

            SubmeshRange operator()(const PrimitiveUpload& up) const {
                SubmeshRange sm{};

                const uint32_t baseVertex = static_cast<uint32_t>(agg->allVerts.size());
                const uint32_t firstIndex = static_cast<uint32_t>(agg->allIdx.size());

                // append vertices
                agg->allVerts.insert(agg->allVerts.end(), up.vertices, up.vertices + up.vertexCount);

                // append indices with baseVertex bias
                agg->allIdx.reserve(agg->allIdx.size() + up.indexCount);
                for (size_t i = 0; i < up.indexCount; ++i)
                    agg->allIdx.push_back(up.indices[i] + baseVertex);

                sm.baseVertex = baseVertex;
                sm.firstIndex = firstIndex;
                sm.indexCount = static_cast<uint32_t>(up.indexCount);
                sm.aabbMin = up.aabbMin;
                sm.aabbMax = up.aabbMax;
                // sm.materialDefaultId is set later by the importer (if any)

                return sm;
            }
        };


	public:
		static void AssetManagerUtils::ComputePivotFromAlpha(const uint8_t* rgba, int w, int h, int alphaThresh,
			int& outPivotYOffsetPx, int& outPivotXCenterOffsetPx);


        
        static inline void ComputeLocalAABB(const std::vector<Vertex>& verts,
            glm::vec3& outMinL,  glm::vec3& outMaxL)
        {
            if (verts.empty()) { outMinL = outMaxL = glm::vec3(0.0f); return; }
            glm::vec3 mn(std::numeric_limits<float>::max());
            glm::vec3 mx(-std::numeric_limits<float>::max());
            for (const auto& v : verts) { mn = glm::min(mn, v.pos); mx = glm::max(mx, v.pos); }
            outMinL = mn; outMaxL = mx;
        }

	};

}

