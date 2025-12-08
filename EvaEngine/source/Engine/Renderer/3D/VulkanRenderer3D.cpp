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
    VulkanRenderer3D::Statistics3D VulkanRenderer3D::s_stats3D;
    uint32_t VulkanRenderer3D::s_debug3DFlags;
   

    void VulkanRenderer3D::ResetStats3D()
    {
        memset(&s_stats3D, 0, sizeof(Statistics3D));
    }

    void VulkanRenderer3D::InitVulkanRenderer3D()
    {

        m_3DRenderShader = std::make_shared<VulkanShader>(AssetManager::GetAssetPath("shaders/Vulkan3DRender.GLSL").string());
        
        
        VulkanContext* vulkanContext = VulkanContext::Get();
        m_device = vulkanContext->GetDeviceManager().GetDevice();
        Create3dDescriptorSetLayout(m_device, m_descriptorSetLayout3D);


        Engine::Vulkan3DGraphicsPipeline::CreateInfo createInfo{};
        createInfo.device = m_device;
        createInfo.renderPass = vulkanContext->GetGameRenderPass();      
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
            // location, binding, format,                             offset
            { 0, 0, VK_FORMAT_R32G32B32_SFLOAT,    offsetof(Vertex, pos)     }, // pos
            { 1, 0, VK_FORMAT_R32G32B32_SFLOAT,    offsetof(Vertex, nrm)     }, // nrm
            { 2, 0, VK_FORMAT_R32G32_SFLOAT,       offsetof(Vertex, uv)      }, // uv
            { 3, 0, VK_FORMAT_R32G32B32A32_UINT,   offsetof(Vertex, joints)  }, // joints
            { 4, 0, VK_FORMAT_R32G32B32A32_SFLOAT, offsetof(Vertex, weights) }  // weights
        };

        Engine::Vulkan3DGraphicsPipeline::RasterState rasterState{};
        rasterState.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        rasterState.polygonMode = VK_POLYGON_MODE_FILL;
        rasterState.cullMode = VK_CULL_MODE_BACK_BIT;
        rasterState.frontFace = VK_FRONT_FACE_CLOCKWISE;
        rasterState.depthTest = VK_TRUE;
        rasterState.depthWrite = VK_TRUE;
        rasterState.depthCompare = VK_COMPARE_OP_LESS_OR_EQUAL;
        rasterState.enableBlending = VK_FALSE; // set true for alpha blended passes

        bool ok = m_3DPipeline.Create(createInfo, shaderStages, vertexInput, rasterState);
        EE_CORE_ASSERT(ok, "Failed to create Vulkan3DGraphicsPipeline");

        
        Init3DBuffers(m_device, MAX_FRAMES_IN_FLIGHT, MAX_3D_INSTANCES, MAX_MATERIALS, m_frames);
        Allocate3DDescriptorSets(vulkanContext->GetDescriptorPool3D());


        //Engine::AssetManager::ImportGLTF(AssetManager::GetAssetFolderPath().string() + "/animations/3D/player/trafficPolice1.glb");
        Engine::AssetManager::ImportGLTF(AssetManager::GetAssetFolderPath().string() + "/animations/3D/player/playerMesh.glb");
        Engine::AssetManager::ImportGLTF(AssetManager::GetAssetFolderPath().string() + "/animations/3D/player/playerAnimRun.glb");
        Engine::AssetManager::ImportGLTF(AssetManager::GetAssetFolderPath().string() + "/animations/3D/player/playerAnimIdle.glb");
        //Engine::AssetManager::ImportGLTF(AssetManager::GetAssetFolderPath().string() + "/animations/3D/player/Engineer.glb");

        for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
        {
            UpdateBonePaletteDesciptorsSet(i);

        }
    }


    bool VulkanRenderer3D::Create3dDescriptorSetLayout(VkDevice device, VkDescriptorSetLayout& descriptorSetLayoutOut)
    {
        // binding 0: Camera UBO (VS & FS)
        VkDescriptorSetLayoutBinding cam{};
        cam.binding = 0;
        cam.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        cam.descriptorCount = 1;
        cam.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
        cam.pImmutableSamplers = nullptr;

        // binding 1: Instance SSBO (VS) -> holds InstanceData[]
        VkDescriptorSetLayoutBinding instances{};
        instances.binding = 1;
        instances.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        instances.descriptorCount = 1;
        instances.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
        instances.pImmutableSamplers = nullptr;

        // binding 2: Albedo texture array (FS)
        VkDescriptorSetLayoutBinding albedoArray{};
        albedoArray.binding = 2;
        albedoArray.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        albedoArray.descriptorCount = MAX_ALBEDO_TEXTURES;
        albedoArray.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
        albedoArray.pImmutableSamplers = nullptr;

        // binding 3: Material buffer (FS)
        VkDescriptorSetLayoutBinding materialBuf{};
        materialBuf.binding = 3;
        materialBuf.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        materialBuf.descriptorCount = 1;
        materialBuf.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
        materialBuf.pImmutableSamplers = nullptr;

        // binding 4: Bone palette SSBO (VS) -> holds mat4 uBones[]
        VkDescriptorSetLayoutBinding bonePalette{};
        bonePalette.binding = 4;
        bonePalette.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        bonePalette.descriptorCount = 1;
        bonePalette.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
        bonePalette.pImmutableSamplers = nullptr;

        VkDescriptorSetLayoutBinding bindings[] = {
            cam,
            instances,
            albedoArray,
            materialBuf,
            bonePalette
        };

        VkDescriptorSetLayoutCreateInfo ci{ VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO };
        ci.bindingCount = static_cast<uint32_t>(std::size(bindings));
        ci.pBindings = bindings;
        ci.flags = 0;
        ci.pNext = nullptr;

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
        VkDescriptorBufferInfo info{ b.GetBuffer(), 0, b.m_size};
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

        uint32_t max_bones = 2000;
        const VkDeviceSize boneBytes = VkDeviceSize(max_bones) * sizeof(glm::vec4);

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

            frames[i].bonePaletteSSBO = VulkanBuffer(
                device, phys, boneBytes,
                VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

            frames[i].bonePaletteSSBO.Map();
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
        if (!src || bytes == 0 || dstOffset >= buf.m_size) return;

        
        if (dstOffset + bytes > buf.m_size)
            bytes = buf.m_size - dstOffset;

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

    void VulkanRenderer3D::UpdateBones(uint32_t frame)
    {
        EE_PROFILE_FUNCTION();

        const auto& bones = s_Vulkan3DData.s_bones;
        if (bones.empty())
            return;

        const VkDeviceSize byteSize = bones.size() * sizeof(glm::mat4);

        UpdateBuffer(m_frames[frame].bonePaletteSSBO,
            bones.data(),
            (size_t)byteSize,
            0);
    }



    void VulkanRenderer3D::UpdateInstances(uint32_t frameIndex,  const InstanceDataGPU* instances, uint32_t count)
    {
        const size_t byteCount = size_t(count) * sizeof(InstanceDataGPU);
        if (byteCount == 0) return;

        void* dst = m_frames[frameIndex].instanceSSBO.Mapped();
        std::memcpy(dst, instances, byteCount);

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
        s_Vulkan3DData.s_bones.clear();
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
            //d.meshId = inst.meshId;
            d.submeshId = submeshFirst + i;
            d.instanceIndex = baseIdx;

            s_Vulkan3DData.s_draws.push_back(d);
        }
    }

    void VulkanRenderer3D::SubmitBone(glm::mat4 bone)
    {
        s_Vulkan3DData.s_bones.push_back(bone);
    }




    void VulkanRenderer3D::Draw(uint32_t frameIndex, VkCommandBuffer cmd)
    {
        EE_PROFILE_FUNCTION();
        UploadMaterials(frameIndex, AssetManager::GetMaterialRegistry());

        UpdateCamera(frameIndex, s_Vulkan3DData.s_cameraData.uView, s_Vulkan3DData.s_cameraData.uProj);
        UpdateBones(frameIndex);

        UpdateAlbedoImageDesciptorsSet(frameIndex);

        const uint32_t numberOfInstances = (uint32_t)s_Vulkan3DData.s_instances.size();
        if (numberOfInstances)
        {
            static std::vector<InstanceDataGPU> tmpInstances;
            tmpInstances.resize(numberOfInstances);

            for (uint32_t i = 0; i < numberOfInstances; ++i)
            {
                const auto& src = s_Vulkan3DData.s_instances[i]; // your CPU instance

                InstanceDataGPU dst{};
                dst.world = src.world;
                dst.boneBase = src.boneBase;   // 0xFFFFFFFFu for non-skinned, valid base for skinned
                dst.boneCount = src.boneCount;
                dst._pad1 = 0;
                dst._pad2 = 0;

                tmpInstances[i] = dst;
            }

            UpdateInstances(frameIndex, tmpInstances.data(), numberOfInstances);
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
        vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_3DPipeline.GetPipeline());
        {
            VkDescriptorSet set0 = m_frames[frameIndex].set0Global;
            vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_3DPipeline.GetLayout(),
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

                s_stats3D.VertexCount += mesh.vertexCount;
                s_stats3D.IndexCount += mesh.indexCount;

            }

            // Push constants for this instance/submesh
            const InstanceDataGPU& instGPU = s_Vulkan3DData.s_instances[d.instanceIndex];

            PCDraw3D pc{};
            pc.instanceIndex = d.instanceIndex;
            //pc.materialId = instGPU.materialId; // or from mesh.submeshes[d.submeshId]
            pc.submeshId = d.submeshId;
            pc.flags = s_debug3DFlags;
            

            vkCmdPushConstants(cmd, m_3DPipeline.GetLayout(), VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
                0, sizeof(PCDraw3D), &pc);

            // Draw this submesh
            const MeshAsset& mesh = meshReg.Get(currentMeshId);
            const SubmeshRange& sm = mesh.submeshes[d.submeshId];

            vkCmdDrawIndexed(cmd, sm.indexCount, 1, sm.firstIndex, static_cast<int32_t>(sm.baseVertex), 0);
     
        }

        // 4) Clear queues for next frame
        s_Vulkan3DData.s_instances.clear();
        s_Vulkan3DData.s_draws.clear();
    }




    void VulkanRenderer3D::Flush3D(const MeshRegistry& meshes, const MaterialRegistry& materials)
    {
        
        
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
            instInfo.range = m_frames[i].instanceSSBO.m_size;

            // materials
            VkDescriptorBufferInfo matInfo{};
            matInfo.buffer = m_frames[i].materialSSBO.GetBuffer();
            matInfo.offset = 0;
            matInfo.range = m_frames[i].materialSSBO.m_size;

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


            VkDescriptorBufferInfo bonesInfo{};
            bonesInfo.buffer = m_frames[i].bonePaletteSSBO.GetBuffer();
            bonesInfo.offset = 0;
            bonesInfo.range = m_frames[i].bonePaletteSSBO.m_size;


            VkWriteDescriptorSet wBones{ VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET };
            wBones.dstSet = m_frames[i].set0Global;
            wBones.dstBinding = 4;
            wBones.dstArrayElement = 0;
            wBones.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
            wBones.descriptorCount = 1;
            wBones.pBufferInfo = &bonesInfo;
            writes.push_back(wBones);

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
        if (m_frames[frame].set0Global == VK_NULL_HANDLE)
            return;

        if (m_albedoImageInfos.empty())
            return;

        VkWriteDescriptorSet w{ VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET };
        w.dstSet = m_frames[frame].set0Global;
        w.dstBinding = 2; // uAlbedo[]
        w.dstArrayElement = 0;
        w.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        w.descriptorCount = static_cast<uint32_t>(m_albedoImageInfos.size());
        w.pImageInfo = m_albedoImageInfos.data();

        vkUpdateDescriptorSets(m_device, 1, &w, 0, nullptr);
    }


    void VulkanRenderer3D::UpdateBonePaletteDesciptorsSet(uint32_t frame)
    {

        for (uint32_t i = 0; i < m_albedoImageInfos.size(); ++i)
        {
            VkDescriptorBufferInfo bonesInfo{};
            bonesInfo.buffer = m_frames[frame].bonePaletteSSBO.GetBuffer();  
            bonesInfo.offset = 0;
            bonesInfo.range = VK_WHOLE_SIZE;

            VkWriteDescriptorSet w{};
            w.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            w.dstSet = m_frames[frame].set0Global;
            w.dstBinding = 4;
            w.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
            w.descriptorCount = 1;
            w.pBufferInfo = &bonesInfo;

            vkUpdateDescriptorSets(m_device, 1, &w, 0, nullptr);
        }
    }

} 
