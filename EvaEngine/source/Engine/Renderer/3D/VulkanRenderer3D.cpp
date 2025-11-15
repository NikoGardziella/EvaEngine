#include "pch.h"

#include "VulkanRenderer3D.h"
#include <Engine/Platform/Vulkan/VulkanContext.h>
#include "Engine/AssetManager/AssetManager.h"

#include <algorithm>
#include <Engine/Platform/Vulkan/VulkanBuffer.h>
#include <Engine/Renderer/VulkanRenderer2D.h>



namespace Engine {

    std::mutex VulkanRenderer3D::s_mutex;
 

    Engine::VulkanRenderer3DData Engine::VulkanRenderer3D::s_Vulkan3DData;

    void VulkanRenderer3D::InitVulkanRenderer3D()
    {

        m_3DRenderShader = std::make_shared<VulkanShader>(AssetManager::GetAssetPath("shaders/Vulkan3DRender.GLSL").string());
        
        
        VulkanContext* vulkanContext = VulkanContext::Get();
        m_device = vulkanContext->GetDeviceManager().GetDevice();
        Create3dDescriptorSetLayout(m_device, m_descriptorSetLayout3D);


        Engine::Vulkan3DGraphicsPipeline::CreateInfo createInfo{};
        createInfo.device = m_device;
        createInfo.renderPass = vulkanContext->GetPresentRenderPass();      // or your forward pass
        //ci.pipelineCache = pipelineCache;       // optional
        createInfo.setLayouts = { m_descriptorSetLayout3D };
        createInfo.pushConstantRange = { VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(PCDraw3D) };
        createInfo.msaaSamples = VK_SAMPLE_COUNT_1_BIT;
        createInfo.colorAttachmentCount = 1;
        createInfo.subpassIndex = 0;

        Engine::Vulkan3DGraphicsPipeline::ShaderStages shaderStages{};
        shaderStages.vert = m_3DRenderShader->GetVertexShaderModule();
        shaderStages.frag = m_3DRenderShader->GetFragmentShaderModule();

        Engine::Vulkan3DGraphicsPipeline::VertexInput vertexInput{};
        vertexInput.bindings =
        {
            { 0, sizeof(Vertex), VK_VERTEX_INPUT_RATE_VERTEX }
        };
        vertexInput.attributes = {
            { 0, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, pos) },
            { 1, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, nrm) },
            { 2, 0, VK_FORMAT_R32G32_SFLOAT, offsetof(Vertex, uv) },
            // add joints/weights if skinned
        };

        Engine::Vulkan3DGraphicsPipeline::RasterState rasterState{};
        rasterState.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        rasterState.polygonMode = VK_POLYGON_MODE_FILL;
        rasterState.cullMode = VK_CULL_MODE_BACK_BIT;
        rasterState.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
        rasterState.depthTest = VK_TRUE;
        rasterState.depthWrite = VK_TRUE;
        rasterState.depthCompare = VK_COMPARE_OP_LESS_OR_EQUAL;
        rasterState.enableBlending = VK_FALSE; // set true for alpha blended passes

        bool ok = m_3DPipeline.Create(createInfo, shaderStages, vertexInput, rasterState);
        EE_CORE_ASSERT(ok, "Failed to create Vulkan3DGraphicsPipeline");

        
        Init3DBuffers(m_device, MAX_FRAMES_IN_FLIGHT, MAX_3D_INSTANCES, m_frames);
        Allocate3DDescriptorSets(vulkanContext->GetDescriptorPool());




        // A tiny unit cube (8 verts, 12 triangles) — positions + normals + uv
        static const Vertex kCubeVerts[] = {
            // pos                nrm           uv
            {{-0.5f,-0.5f,-0.5f}, { 0, 0,-1}, {0,0}},
            {{ 0.5f,-0.5f,-0.5f}, { 0, 0,-1}, {1,0}},
            {{ 0.5f, 0.5f,-0.5f}, { 0, 0,-1}, {1,1}},
            {{-0.5f, 0.5f,-0.5f}, { 0, 0,-1}, {0,1}},
            {{-0.5f,-0.5f, 0.5f}, { 0, 0, 1}, {0,0}},
            {{ 0.5f,-0.5f, 0.5f}, { 0, 0, 1}, {1,0}},
            {{ 0.5f, 0.5f, 0.5f}, { 0, 0, 1}, {1,1}},
            {{-0.5f, 0.5f, 0.5f}, { 0, 0, 1}, {0,1}},
        };
        static const uint32_t kCubeIdx[] = {
            0,1,2, 2,3,0,  // back
            4,5,6, 6,7,4,  // front
            0,4,7, 7,3,0,  // left
            1,5,6, 6,2,1,  // right
            3,2,6, 6,7,3,  // top
            0,1,5, 5,4,0   // bottom
        };

        // Create VB/IB (host-visible is fine for now)
        vb = new Engine::VulkanVertexBuffer((float*)kCubeVerts, sizeof(kCubeVerts));
        ib = new Engine::VulkanIndexBuffer((uint32_t*)kCubeIdx, (uint32_t)std::size(kCubeIdx));


        Engine::AssetManager::ImportGLTF(AssetManager::GetAssetFolderPath().string() + "/animations/3D/player/human.glb");

    }

    bool VulkanRenderer3D::Create3dDescriptorSetLayout(VkDevice device, VkDescriptorSetLayout& descriptorSetLayoutOut)
    {
        // set = 0

        // binding 0: Camera UBO (VS & FS)
        VkDescriptorSetLayoutBinding cam{};
        cam.binding = 0;
        cam.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        cam.descriptorCount = 1;
        cam.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;

        // binding 1: Instance SSBO (VS) -> holds mat4 world[]
        VkDescriptorSetLayoutBinding instances{};
        instances.binding = 1;
        instances.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        instances.descriptorCount = 1;
        instances.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

        VkDescriptorSetLayoutBinding bindings[] = { cam, instances };

        VkDescriptorSetLayoutCreateInfo ci{ VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO };
        ci.bindingCount = static_cast<uint32_t>(std::size(bindings));
        ci.pBindings = bindings;

        return vkCreateDescriptorSetLayout(device, &ci, nullptr, &descriptorSetLayoutOut) == VK_SUCCESS;
    }



    VkDescriptorBufferInfo VulkanRenderer3D::CameraInfo(uint32_t frame) const 
    {
        const auto& b = m_frames[frame].cameraUBO;
        VkDescriptorBufferInfo info{ b.GetBuffer(), 0, sizeof(CameraUBO)};
        return info;
    }

    VkDescriptorBufferInfo VulkanRenderer3D::InstanceInfo(uint32_t frame) const 
    {
        const auto& b = m_frames[frame].instanceSSBO;
        VkDescriptorBufferInfo info{ b.GetBuffer(), 0, b.size};
        return info;
    }

    bool VulkanRenderer3D::Init3DBuffers(VkDevice device, uint32_t framesInFlight,
        uint32_t maxInstances, std::vector<Renderer3DPerFrame>& frames)
    {
        VulkanContext* ctx = VulkanContext::Get();
        VkPhysicalDevice phys = ctx->GetDeviceManager().GetPhysicalDevice();

        frames.resize(framesInFlight);

        // Camera UBO: struct { mat4 view; mat4 proj; }  (or use 1 mat4 if you prefer ViewProj)
        const VkDeviceSize camBytes = sizeof(CameraUBO);

        // Instance SSBO: mat4 world[maxInstances]
        const VkDeviceSize instBytes = VkDeviceSize(maxInstances) * sizeof(glm::mat4);

        for (uint32_t i = 0; i < framesInFlight; ++i) {
            frames[i].cameraUBO = VulkanBuffer(
                device, phys, camBytes,
                VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

            frames[i].cameraUBO.Map();

            frames[i].instanceSSBO = VulkanBuffer(
                device, phys, instBytes,
                VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

            frames[i].instanceSSBO.Map();
        }
        return true;
    }



    void VulkanRenderer3D::UpdateBuffer(const VulkanBuffer& buf, const void* src, VkDeviceSize bytes, VkDeviceSize dstOffset /*= 0*/) const
    {
        if (!src || bytes == 0 || dstOffset >= buf.size) return;

        
        if (dstOffset + bytes > buf.size)
            bytes = buf.size - dstOffset;

        // must be persistently mapped
        void* base = buf.Mapped();
        EE_CORE_ASSERT(base, "UpdateBuffer: buffer must be persistently mapped before use.");

        // copy into mapped memory
        std::memcpy(static_cast<char*>(base) + static_cast<size_t>(dstOffset), src, static_cast<size_t>(bytes));

        VkMappedMemoryRange rng{ VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE };
        rng.memory = buf.GetMemory();
        rng.offset = dstOffset;
        rng.size = bytes;
        vkFlushMappedMemoryRanges(m_device, 1, &rng);
    }


    void VulkanRenderer3D::UpdateCamera(uint32_t frame,const glm::mat4& view, const glm::mat4& proj)
    {
        EE_PROFILE_FUNCTION();

        CameraUBO cam{ view, proj };
        UpdateBuffer(m_frames[frame].cameraUBO, &cam, sizeof(cam), 0);

        
    }

    void VulkanRenderer3D::UpdateInstances(uint32_t frame,const glm::mat4* worlds, uint32_t count)
    {
        EE_PROFILE_FUNCTION();

        if (!worlds || count == 0) return;
        VkDeviceSize bytes = VkDeviceSize(count) * sizeof(glm::mat4);
        UpdateBuffer(m_frames[frame].instanceSSBO, worlds, bytes, 0);
    }

    void VulkanRenderer3D::Begin3DScene(const  glm::mat4& projection, const  glm::mat4& view, const glm::mat4& cameraWorld)
    {
        EE_PROFILE_FUNCTION();

  
        const glm::mat4 V = glm::inverse(cameraWorld);
  

        s_Vulkan3DData.s_cameraData.uProj = projection;
        s_Vulkan3DData.s_cameraData.uView = V;


    }

    void VulkanRenderer3D::BeginFrame3D(uint32_t frameIndex)
    {
        EE_PROFILE_FUNCTION();

       
        s_Vulkan3DData.s_instances.clear();
        s_Vulkan3DData.s_draws.clear();

        // (Optional) reserve to limit reallocs
        // s_instances.reserve(4096);
        // s_draws.reserve(8192);
        
    }

    void VulkanRenderer3D::SubmitMeshInstance(const InstanceDataGPU& inst, uint32_t submeshId)
    {
        EE_PROFILE_FUNCTION();

        std::scoped_lock lock(s_mutex);
        uint32_t idx = (uint32_t)s_Vulkan3DData.s_instances.size();
        s_Vulkan3DData.s_instances.push_back(inst);
        s_Vulkan3DData.s_draws.push_back(PendingDraw{ idx, submeshId });
    }

    void VulkanRenderer3D::SubmitMeshInstanceRange(const InstanceDataGPU& inst, uint32_t submeshFirst, uint32_t submeshCount)
    {
        //EE_PROFILE_FUNCTION();

        std::scoped_lock lock(s_mutex);
        uint32_t baseIdx = (uint32_t)s_Vulkan3DData.s_instances.size();
        s_Vulkan3DData.s_instances.push_back(inst);
        for (uint32_t i = 0; i < submeshCount; ++i)
        {
            s_Vulkan3DData.s_draws.push_back(PendingDraw{ baseIdx, submeshFirst + i });
        }
    }


 
    void VulkanRenderer3D::Draw(uint32_t frameIndex, VkCommandBuffer cmd)
    {
        EE_PROFILE_FUNCTION();


        UpdateCamera(frameIndex, s_Vulkan3DData.s_cameraData.uView, s_Vulkan3DData.s_cameraData.uProj);


        const uint32_t N = (uint32_t)s_Vulkan3DData.s_instances.size();
        if (N) 
        {
            static std::vector<glm::mat4> tmpWorlds;
            tmpWorlds.resize(N);
            for (uint32_t i = 0; i < N; ++i)
                tmpWorlds[i] = s_Vulkan3DData.s_instances[i].world;

            UpdateInstances(frameIndex, tmpWorlds.data(), N);
        }
        // 1) Upload instances -> set=0,binding=1 SSBO
        /*
        {
            const size_t byteCount = s_Vulkan3DData.s_instances.size() * sizeof(InstanceDataGPU);
            if (byteCount > 0)
            {
                void* dst = m_frames[frameIndex].instanceSSBO.Mapped(); // persistently mapped in your init
                std::memcpy(dst, s_Vulkan3DData.s_instances.data(), byteCount);

                VkMappedMemoryRange rng{ VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE };
                rng.memory = m_frames[frameIndex].instanceSSBO.GetMemory();
                rng.offset = 0;
                rng.size = VK_WHOLE_SIZE;
                vkFlushMappedMemoryRanges(m_device, 1, &rng);


            }
        }
        */
        const uint32_t meshId = 0;  // set this before SubmitMeshInstanceRange()
        const MeshAsset& mesh = AssetManager::GetMeshRegistry().Get(meshId);


        // 2) Bind pipeline + descriptor set 0 (camera + instances)
        vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_3DPipeline.Get());
        {
            VkDescriptorSet set0 = m_frames[frameIndex].set0Global;
            vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS,
                m_3DPipeline.GetLayout(),
                0, 1, &set0, 0, nullptr);
        }

        // 3) Bind the active mesh VB/IB once
        // NOTE: replace AssetManager::GetMeshRegistry() with your actual accessor
        VkDeviceSize vbOff = mesh.vbOffset;
        vkCmdBindVertexBuffers(cmd, 0, 1, &mesh.vertexBuffer, &vbOff);
        vkCmdBindIndexBuffer(cmd, mesh.indexBuffer, mesh.ibOffset, VK_INDEX_TYPE_UINT32);

        // 4) Record draws for queued submeshes
        // Optional: sort by submesh to improve locality
        // std::sort(s_Vulkan3DData.s_draws.begin(), s_Vulkan3DData.s_draws.end(),
        //           [](const PendingDraw& a, const PendingDraw& b){ return a.submeshIndex < b.submeshIndex; });

        for (const PendingDraw& d : s_Vulkan3DData.s_draws)
        {
            // push constants: {instanceIndex, materialId, submeshId, flags}
            // materialId is in InstanceDataGPU; if you want per-submesh, look it up from mesh.submeshes[d.submeshIndex]
            PCDraw3D pc{};
            pc.instanceIndex = d.instanceIndex;
            pc.materialId = s_Vulkan3DData.s_instances[d.instanceIndex].materialId;
            pc.submeshId = d.submeshId;
            pc.flags = s_Vulkan3DData.s_instances[d.instanceIndex].flags;

            vkCmdPushConstants(cmd, m_3DPipeline.GetLayout(),
                VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
                0, sizeof(PCDraw3D), &pc);

            const SubmeshRange& sm = mesh.submeshes[d.submeshId];
            vkCmdDrawIndexed(cmd, sm.indexCount, 1, sm.firstIndex, static_cast<int32_t>(sm.baseVertex), 0);
        }
       
     
        // 5) Clear per-frame queues for next frame
        s_Vulkan3DData.s_instances.clear();
        s_Vulkan3DData.s_draws.clear();
    }



    void VulkanRenderer3D::Flush3D(const MeshRegistry& meshes, const MaterialRegistry& materials)
    {
        
        // This is where you:
        // 1) Upload s_instances to your per-frame InstanceData SSBO
        // 2) Optionally sort s_draws by material/pipeline key
        // 3) Build indirect draw buffers (or issue direct draws)
        // 4) Record Vulkan commands

        // --- 1) Upload instances ---
        // Map your instance SSBO and copy s_instances.data(), size = s_instances.size()*sizeof(InstanceDataGPU)
        // (pseudocode) UploadInstanceSSBO(s_instances);

        // --- 2) Optional sort by material (example) ---
        // std::stable_sort(s_draws.begin(), s_draws.end(), [&](auto& a, auto& b){
        //     // material row comes from instance.materialId
        //     return s_instances[a.instanceIndex].materialId < s_instances[b.instanceIndex].materialId;
        // });

        // --- 3/4) Issue draws ---
        // for (auto& d : s_draws) {
        //     const InstanceDataGPU& inst = s_instances[d.instanceIndex];
        //     const SubmeshRange& sub = meshes.LookupSubmesh(d.submeshId); // implement GetSubmesh(id) in your registry
        //
        //     // Bind pipelines & descriptor sets as needed (material table, textures, bone palette)
        //     // Set push constants: instanceIndex, submeshId, (optional) materialId
        //     // Bind VB/IB (or use device address if you do so)
        //     // vkCmdDrawIndexed(... sub.indexCount, 1, sub.firstIndex, sub.vertexOffset, 0);
        // }

        // Nothing to clear here; BeginFrame3D() will reset for the next frame.
    }


    bool VulkanRenderer3D::Allocate3DDescriptorSets(VkDescriptorPool pool)
    {
        for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
        {
            // allocate one set per frame
            VkDescriptorSetAllocateInfo ai{ VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO };
            ai.descriptorPool = pool;
            ai.descriptorSetCount = 1;
            ai.pSetLayouts = &m_descriptorSetLayout3D;

            VkResult r = vkAllocateDescriptorSets(m_device, &ai, &m_frames[i].set0Global);
            if (r != VK_SUCCESS) return false;

            // write binding 0 (camera UBO) + binding 1 (instance SSBO)
            VkDescriptorBufferInfo camInfo{};
            camInfo.buffer = m_frames[i].cameraUBO.GetBuffer();
            camInfo.offset = 0;
            camInfo.range = sizeof(glm::mat4) * 2; // {view, proj}  (match your CameraUBO)

            VkDescriptorBufferInfo instInfo{};
            instInfo.buffer = m_frames[i].instanceSSBO.GetBuffer();
            instInfo.offset = 0;
            instInfo.range = m_frames[i].instanceSSBO.size; // capacity in bytes

            VkWriteDescriptorSet w[2]{};
            w[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            w[0].dstSet = m_frames[i].set0Global;
            w[0].dstBinding = 0; // Camera UBO
            w[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
            w[0].descriptorCount = 1;
            w[0].pBufferInfo = &camInfo;

            w[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            w[1].dstSet = m_frames[i].set0Global;
            w[1].dstBinding = 1; // Instance SSBO
            w[1].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
            w[1].descriptorCount = 1;
            w[1].pBufferInfo = &instInfo;

            vkUpdateDescriptorSets(m_device, 2, w, 0, nullptr);
        }
        return true;
    }


} // namespace Engine
