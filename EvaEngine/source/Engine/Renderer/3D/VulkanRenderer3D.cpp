#include "pch.h"

#include "VulkanRenderer3D.h"
#include <Engine/Platform/Vulkan/VulkanContext.h>
#include "Engine/AssetManager/AssetManager.h"

#include <algorithm>
#include <Engine/Platform/Vulkan/VulkanBuffer.h>
#include <Engine/Renderer/VulkanRenderer2D.h>
#include <Engine/Animation/3D/MaterialRegistry.h>



namespace Engine {

    std::mutex VulkanRenderer3D::s_mutex;
 

    Engine::VulkanRenderer3DData Engine::VulkanRenderer3D::s_Vulkan3DData;
    std::vector<VkDescriptorImageInfo> VulkanRenderer3D::m_albedoImageInfos;
    std::vector<Ref<VulkanTexture>> VulkanRenderer3D::m_albedoTextures;

    void VulkanRenderer3D::InitVulkanRenderer3D()
    {

        m_3DRenderShader = std::make_shared<VulkanShader>(AssetManager::GetAssetPath("shaders/Vulkan3DRender.GLSL").string());
        
        
        VulkanContext* vulkanContext = VulkanContext::Get();
        m_device = vulkanContext->GetDeviceManager().GetDevice();
        Create3dDescriptorSetLayout(m_device, m_descriptorSetLayout3D);


        Engine::Vulkan3DGraphicsPipeline::CreateInfo createInfo{};
        createInfo.device = m_device;
        createInfo.renderPass = vulkanContext->GetPresentRenderPass();      
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

        
       

        Init3DBuffers(m_device, MAX_FRAMES_IN_FLIGHT, MAX_3D_INSTANCES, MAX_MATERIALS, m_frames);
        Allocate3DDescriptorSets(vulkanContext->GetDescriptorPool3D());




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


        Engine::AssetManager::ImportGLTF(AssetManager::GetAssetFolderPath().string() + "/animations/3D/player/trafficPolice1.glb");
        Engine::AssetManager::ImportGLTF(AssetManager::GetAssetFolderPath().string() + "/animations/3D/player/Engineer.glb");


        // somewhere on init
        Ref<VulkanTexture> dbgTex = std::make_shared<VulkanTexture>(
            64, 64, VK_FORMAT_R8G8B8A8_UNORM, false);

      
     



    }

    bool VulkanRenderer3D::Create3dDescriptorSetLayout(VkDevice device, VkDescriptorSetLayout& descriptorSetLayoutOut)
    {
        // binding 0: Camera UBO (VS & FS)
        VkDescriptorSetLayoutBinding cam{};
        cam.binding = 0;
        cam.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        cam.descriptorCount = 1;
        cam.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;

        // binding 1: Instance SSBO (VS) -> holds InstanceData[]
        VkDescriptorSetLayoutBinding instances{};
        instances.binding = 1;
        instances.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        instances.descriptorCount = 1;
        instances.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

        // binding 2: Albedo texture array (FS)
        VkDescriptorSetLayoutBinding albedoArray{};
        albedoArray.binding = 2;
        albedoArray.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        albedoArray.descriptorCount = 1;
        albedoArray.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

        // binding 3: Material buffer (FS)
        VkDescriptorSetLayoutBinding materialBuf{};
        materialBuf.binding = 3;
        materialBuf.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        materialBuf.descriptorCount = 1;
        materialBuf.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

        VkDescriptorSetLayoutBinding bindings[] = {
            cam,
            instances,
            albedoArray,
            materialBuf
        };

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

    bool VulkanRenderer3D::Init3DBuffers(VkDevice device, uint32_t framesInFlight, uint32_t maxInstances,
        uint32_t maxMaterials, std::vector<Renderer3DPerFrame>& frames)
    {
        VulkanContext* ctx = VulkanContext::Get();
        VkPhysicalDevice phys = ctx->GetDeviceManager().GetPhysicalDevice();

        frames.resize(framesInFlight);

        // Camera UBO: struct { mat4 view; mat4 proj; }
        const VkDeviceSize camBytes = sizeof(CameraUBO);

        // Instance SSBO: InstanceDataGPU[maxInstances]
        const VkDeviceSize instBytes = VkDeviceSize(maxInstances) * sizeof(InstanceDataGPU);

        // Material SSBO: MaterialGPU[maxMaterials]
        const VkDeviceSize matBytes = VkDeviceSize(maxMaterials) * sizeof(MaterialGPU);

        for (uint32_t i = 0; i < framesInFlight; ++i)
        {
            // Camera
            frames[i].cameraUBO = VulkanBuffer(
                device, phys, camBytes,
                VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

            frames[i].cameraUBO.Map();

            // Instances
            frames[i].instanceSSBO = VulkanBuffer(
                device, phys, instBytes,
                VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

            frames[i].instanceSSBO.Map();

            // Materials
            frames[i].materialSSBO = VulkanBuffer(
                device, phys, matBytes,
                VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

            frames[i].materialSSBO.Map();
        }

        return true;
    }


    void VulkanRenderer3D::UploadMaterials(uint32_t frameIndex,
        const MaterialRegistry& materials)
    {
        const auto& gpuMats = materials.GPUTable();
        if (gpuMats.empty())
            return;

        Renderer3DPerFrame& frame = m_frames[frameIndex];

        const VkDeviceSize byteCount =
            VkDeviceSize(gpuMats.size()) * sizeof(MaterialGPU);

        // safety: don't overflow the buffer
      
        void* dst = frame.materialSSBO.Mapped();
        std::memcpy(dst, gpuMats.data(), (size_t)byteCount);

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

    void VulkanRenderer3D::Begin3DScene(const  glm::mat4& projection, const  glm::mat4& view)
    {
        EE_PROFILE_FUNCTION();

  
        //const glm::mat4 V = glm::inverse(cameraWorld);
  

        s_Vulkan3DData.s_cameraData.uProj = projection;
        s_Vulkan3DData.s_cameraData.uView = view;


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
            PendingDraw d{};
            d.meshId = inst.meshId;
            d.submeshId = submeshFirst + i;
            d.instanceIndex = baseIdx;

            s_Vulkan3DData.s_draws.push_back(d);
        }
    }


    void VulkanRenderer3D::Draw(uint32_t frameIndex, VkCommandBuffer cmd)
    {
        EE_PROFILE_FUNCTION();
        UploadMaterials(frameIndex, AssetManager::GetMaterialRegistry());
        UpdateCamera(frameIndex, s_Vulkan3DData.s_cameraData.uView, s_Vulkan3DData.s_cameraData.uProj);
        UpdateAlbedoImageDesciptorsSet(frameIndex);

        const uint32_t numberOfInstances = (uint32_t)s_Vulkan3DData.s_instances.size();
        if (numberOfInstances)
        {
            static std::vector<glm::mat4> tmpWorlds;
            tmpWorlds.resize(numberOfInstances);
            for (uint32_t i = 0; i < numberOfInstances; ++i)
                tmpWorlds[i] = s_Vulkan3DData.s_instances[i].world;

            UpdateInstances(frameIndex, tmpWorlds.data(), numberOfInstances);
        }

        // Nothing to draw
        if (s_Vulkan3DData.s_draws.empty())
        {
            s_Vulkan3DData.s_instances.clear();
            return;
        }

        // 1) Sort draws by meshId, then by submeshId (optional, but nice)
        std::sort(s_Vulkan3DData.s_draws.begin(), s_Vulkan3DData.s_draws.end(),
            [](const PendingDraw& a, const PendingDraw& b)
            {
                if (a.meshId != b.meshId)   return a.meshId < b.meshId;
                if (a.submeshId != b.submeshId) return a.submeshId < b.submeshId;
                return a.instanceIndex < b.instanceIndex;
            });

        // 2) Bind pipeline + global set0 (camera + instances + materials)
        vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_3DPipeline.Get());
        {
            VkDescriptorSet set0 = m_frames[frameIndex].set0Global;
            vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS,
                m_3DPipeline.GetLayout(),
                0, 1, &set0, 0, nullptr);
        }

        uint32_t currentMeshId = UINT32_MAX;
        const MeshRegistry& meshReg = AssetManager::GetMeshRegistry();

        // 3) Walk all draws
        for (const PendingDraw& d : s_Vulkan3DData.s_draws)
        {
            // Rebind VB/IB when mesh changes
            if (d.meshId != currentMeshId)
            {
                currentMeshId = d.meshId;
                const MeshAsset& mesh = meshReg.Get(currentMeshId);

                VkDeviceSize vbOff = mesh.vbOffset;
                vkCmdBindVertexBuffers(cmd, 0, 1, &mesh.vertexBuffer, &vbOff);
                vkCmdBindIndexBuffer(cmd, mesh.indexBuffer, mesh.ibOffset, VK_INDEX_TYPE_UINT32);
            }

            // Push constants for this instance/submesh
            const InstanceDataGPU& instGPU = s_Vulkan3DData.s_instances[d.instanceIndex];

            PCDraw3D pc{};
            pc.instanceIndex = d.instanceIndex;
            pc.materialId = instGPU.materialId; // or from mesh.submeshes[d.submeshId]
            pc.submeshId = d.submeshId;
            pc.flags = instGPU.flags;

            vkCmdPushConstants(cmd, m_3DPipeline.GetLayout(),
                VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
                0, sizeof(PCDraw3D), &pc);

            // Draw this submesh
            const MeshAsset& mesh = meshReg.Get(currentMeshId);
            const SubmeshRange& sm = mesh.submeshes[d.submeshId];

            vkCmdDrawIndexed(cmd,
                sm.indexCount,
                1,                       // instanceCount = 1 (you are using instanceIndex via PC)
                sm.firstIndex,
                static_cast<int32_t>(sm.baseVertex),
                0);
        }

        // 4) Clear queues for next frame
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
            VkDescriptorSetAllocateInfo ai{ VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO };
            ai.descriptorPool = pool;
            ai.descriptorSetCount = 1;
            ai.pSetLayouts = &m_descriptorSetLayout3D;

            if (vkAllocateDescriptorSets(m_device, &ai, &m_frames[i].set0Global) != VK_SUCCESS)
                return false;

            // camera
            VkDescriptorBufferInfo camInfo{};
            camInfo.buffer = m_frames[i].cameraUBO.GetBuffer();
            camInfo.offset = 0;
            camInfo.range = sizeof(CameraUBO);

            // instances
            VkDescriptorBufferInfo instInfo{};
            instInfo.buffer = m_frames[i].instanceSSBO.GetBuffer();
            instInfo.offset = 0;
            instInfo.range = m_frames[i].instanceSSBO.size;

            // materials
            VkDescriptorBufferInfo matInfo{};
            matInfo.buffer = m_frames[i].materialSSBO.GetBuffer();
            matInfo.offset = 0;
            matInfo.range = m_frames[i].materialSSBO.size;

            std::vector<VkWriteDescriptorSet> writes;

            // binding 0: cam
            VkWriteDescriptorSet wCam{ VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET };
            wCam.dstSet = m_frames[i].set0Global;
            wCam.dstBinding = 0;
            wCam.dstArrayElement = 0;
            wCam.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
            wCam.descriptorCount = 1;
            wCam.pBufferInfo = &camInfo;
            writes.push_back(wCam);

            // binding 1: instances
            VkWriteDescriptorSet wInst{ VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET };
            wInst.dstSet = m_frames[i].set0Global;
            wInst.dstBinding = 1;
            wInst.dstArrayElement = 0;
            wInst.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
            wInst.descriptorCount = 1;
            wInst.pBufferInfo = &instInfo;
            writes.push_back(wInst);

            // binding 2: albedo texture array (only if some textures exist)
            if (!m_albedoImageInfos.empty())
            {
                VkWriteDescriptorSet wAlbedo{ VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET };
                wAlbedo.dstSet = m_frames[i].set0Global;
                wAlbedo.dstBinding = 2;
                wAlbedo.dstArrayElement = 0;
                wAlbedo.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
                wAlbedo.descriptorCount = 1;
                wAlbedo.pImageInfo = &m_albedoImageInfos[0]; // use slot 0 for test
                writes.push_back(wAlbedo);
            }

            // binding 3: materials
            VkWriteDescriptorSet wMat{ VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET };
            wMat.dstSet = m_frames[i].set0Global;
            wMat.dstBinding = 3;
            wMat.dstArrayElement = 0;
            wMat.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
            wMat.descriptorCount = 1;
            wMat.pBufferInfo = &matInfo;
            writes.push_back(wMat);

            vkUpdateDescriptorSets(m_device,
                (uint32_t)writes.size(),
                writes.data(),
                0, nullptr);
        }
        return true;
    }


    uint32_t VulkanRenderer3D::RegisterAlbedoTexture(const Ref<VulkanTexture>& tex)
    {
        // keep texture alive as long as renderer3D lives
        uint32_t index = (uint32_t)m_albedoTextures.size();
        m_albedoTextures.push_back(tex);

        VkDescriptorImageInfo info{};
        info.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        info.imageView = tex->GetImageView();
        info.sampler = tex->GetSampler();

        if (index >= m_albedoImageInfos.size())
            m_albedoImageInfos.push_back(info);
        else
            m_albedoImageInfos[index] = info;


        return index;
    }


    void VulkanRenderer3D::UpdateAlbedoImageDesciptorsSet(uint32_t frame)
    {
       
        // If descriptor sets are already allocated, update this one slot in the array
        for (uint32_t i = 0; i < m_albedoImageInfos.size(); ++i)
        {
            if (m_frames[frame].set0Global == VK_NULL_HANDLE)
                continue; // not allocated yet

            VkWriteDescriptorSet w{ VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET };
            w.dstSet = m_frames[frame].set0Global;
            w.dstBinding = 2;               // binding for uAlbedoArray[]
            w.dstArrayElement = 0;           // this array index
            w.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
            w.descriptorCount = 1;
            w.pImageInfo = &m_albedoImageInfos[0];

            vkUpdateDescriptorSets(m_device, 1, &w, 0, nullptr);
        }
    }


} 
