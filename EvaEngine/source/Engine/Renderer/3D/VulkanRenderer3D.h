#pragma once
#include <vector>
#include <mutex>
#include <cstdint>
#include "glm/glm.hpp"
#include "Vulkan3DGraphicsPipeline.h"
#include <Engine/Platform/Vulkan/VulkanShader.h>
#include <Engine/Platform/Vulkan/VulkanBuffer.h>
#include <Engine/Renderer/Camera.h>
#include <Engine/Core/Core.h>

namespace Engine {





    struct InstanceDataGPU
    {
        glm::mat4 world;
        uint32_t  boneBase;
        uint32_t  boneCount;
        uint32_t  _pad1;
        uint32_t  _pad2;
    };


 


    struct PendingDraw
    {
        uint32_t instanceIndex;
        uint32_t submeshId;
        uint32_t meshId;
    };

    struct CameraUBO
    {
        glm::mat4 uView;
        glm::mat4 uProj;
    };
    static_assert(sizeof(CameraUBO) == 128, "Camera UBO must be 128 bytes");

    struct VulkanRenderer3DData
    {
        std::vector<InstanceDataGPU> s_instances;
        std::vector<PendingDraw>     s_draws;
        std::vector<glm::mat4>       s_bones;
        CameraUBO                    s_cameraData;
    };

    class VulkanTexture;
    class MeshRegistry;
    class MaterialRegistry;
    class VulkanRenderer3D
    {
    private:
        
        struct Renderer3DPerFrame 
        {
            VulkanBuffer cameraUBO;   // set=0, binding=0
            VulkanBuffer instanceSSBO; // set=0, binding=1
            VulkanBuffer materialSSBO;
            VulkanBuffer bonePaletteSSBO;
            VkDescriptorSet set0Global = VK_NULL_HANDLE;
        };

        

        struct Vertex {
            glm::vec3 pos;   // location = 0  -> VK_FORMAT_R32G32B32_SFLOAT
            glm::vec3 nrm;   // location = 1  -> VK_FORMAT_R32G32B32_SFLOAT
            glm::vec2 uv;    // location = 2  -> VK_FORMAT_R32G32_SFLOAT
            glm::uvec4 joints; // location 3 (JOINTS_0)
            glm::vec4  weights;// location 4 (WEIGHTS_0)
        };

       
        static_assert(offsetof(Vertex, pos) == 0 && offsetof(Vertex, nrm) == 12 && offsetof(Vertex, uv) == 24);

        struct PCDraw3D {
            uint32_t instanceIndex;
            uint32_t materialId;
            uint32_t submeshId;
            uint32_t flags;
        };
        static_assert(sizeof(PCDraw3D) == 16, "Expect 16 bytes");



    public:
        struct Statistics3D
        {
            uint32_t DrawCalls = 0;
            uint32_t QuadCount = 0;
            uint32_t VertexCount = 0;
            uint32_t IndexCount = 0;
   
            uint32_t GetQuadCount() { return QuadCount; }
            uint32_t GetVertexCount() { return VertexCount; }
            uint32_t GetIndexCount() { return IndexCount; }
            uint32_t GetDrawCalls() { return DrawCalls; }
        };
        static Statistics3D GetStats3D() { return s_stats3D; }
        static void ResetStats3D();


    public:

        void InitVulkanRenderer3D();

        bool Create3dDescriptorSetLayout(VkDevice device, VkDescriptorSetLayout& descriptorSetLayoutOut);


        // per-frame updates
    
        // descriptor infos for binding
        const VkDescriptorSet GetSet0(uint32_t frame) const { return m_frames[frame].set0Global; }
        VkDescriptorBufferInfo CameraInfo(uint32_t frame) const;
        VkDescriptorBufferInfo InstanceInfo(uint32_t frame) const;

        bool Init3DBuffers(VkDevice device, uint32_t framesInFlight, uint32_t maxInstances, uint32_t maxMaterials, std::vector<Renderer3DPerFrame>& frames);

        void UploadMaterials(uint32_t frameIndex, const MaterialRegistry& materials);


        //static void Begin3DScene(const Camera& camera, const glm::mat4& transform);


        void UpdateCamera(uint32_t frame, const glm::mat4& view, const glm::mat4& proj);

        void UpdateBones(uint32_t frame);

        void UpdateInstances(uint32_t frameIndex, const InstanceDataGPU* instances, uint32_t count);

        void UpdateInstances(uint32_t frame,  const glm::mat4* worlds, uint32_t count);

        static void Begin3DScene(const glm::mat4& projection, const  glm::mat4& view);

        // Call once per frame, before any Submit* calls

        void BeginFrame3D(uint32_t frameIndex);

        // Submit a single submesh draw with its own InstanceData
        static void SubmitMeshInstance(const InstanceDataGPU& inst, uint32_t submeshId);

        // Convenience: submit a range of submeshes from [first, first+count)
        static void SubmitMeshInstanceRange(const InstanceDataGPU& inst, uint32_t submeshFirst, uint32_t submeshCount);

        static void SubmitBone(glm::mat4 bone);

        void Draw(uint32_t frameIndex, VkCommandBuffer cmd);

        // Record and upload everything. Call from your render pass code.
        // You already have MeshRegistry/MaterialRegistry in your renderer, so pass them in.
        void Flush3D(const MeshRegistry& meshes, const MaterialRegistry& materials);

        bool Allocate3DDescriptorSets(VkDescriptorPool pool);


        static uint32_t RegisterAlbedoTexture(const Ref<VulkanTexture>& tex);

        void UpdateAlbedoImageDesciptorsSet(uint32_t frame);

        void UpdateBonePaletteDesciptorsSet(uint32_t frame);

        static void SetDebugFlags(uint32_t flags) { s_debug3DFlags = flags; }
        static uint32_t GetDebugFlags() { return s_debug3DFlags; }

    private:
       inline void UpdateBuffer(const VulkanBuffer& buf, const void* src, VkDeviceSize bytes, VkDeviceSize dstOffset) const;


    private:
       

        static std::mutex s_mutex;
        static VulkanRenderer3DData s_Vulkan3DData;
        static std::vector<VkDescriptorImageInfo> m_albedoImageInfos;
        static std::vector<Ref<VulkanTexture>>      m_albedoTextures;


        Vulkan3DGraphicsPipeline m_3DPipeline;
        Ref<VulkanShader> m_3DRenderShader;
        VkDevice m_device;
        VkDescriptorSetLayout m_descriptorSetLayout3D;



        VkDevice m_dev = VK_NULL_HANDLE;
        VkPhysicalDevice m_phys = VK_NULL_HANDLE;
        std::vector<Renderer3DPerFrame> m_frames;
        uint32_t m_maxInstances = 0;

        //remove
        VulkanVertexBuffer* vb;
        VulkanIndexBuffer* ib;



        //stats

        static Statistics3D s_stats3D;

       
        static uint32_t s_debug3DFlags;
    };

}