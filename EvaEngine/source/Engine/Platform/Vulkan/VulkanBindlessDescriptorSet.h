#pragma once

#include "Engine.h" 
#include <vector>
#include <array>
#include <unordered_map>
#include <memory>

#include "LayerPool/LayerPool.h"
#include <Engine/Map/TextureStreaming/TextureStreamingSystem.h>

#include <cstdint>
#include <cstddef>
#include <Engine/Renderer/Lights/Shadow/VulkanShadowMap.h>



namespace Engine {

    class Camera;
    class VulkanTexture;
    class VulkanShader; // fwd
    class VulkanBindlessDescriptorSetRenderer {

    public:
        struct TilePC
        {
            glm::mat4 VP;
            glm::mat4 LightSpaceMatrix;
        };


        struct ShadowPC
        {
            glm::vec4 LightDirecion;
            glm::mat4 LightSpaceMatrix;
        };


    private:
        struct SpriteRec {
            uint32_t slot = 0xFFFFFFFFu;
            uint32_t w = 0, h = 0;
            uint32_t refcount = 0;
            Ref<VulkanTexture> tex; // keep GPU image/view alive
        };

        // --------- Instance buffer triple-buffered
        struct InstanceBuffer {
            VkBuffer        buf[3]{};
            VkDeviceMemory  mem[3]{};
            void* mapped[3]{};
            VkDeviceSize    capacityBytes = 0;
        };

        // --------- Render instance (std430-friendly, 32 bytes)
        struct RenderInstance {
            glm::vec2 worldPos;  // 0..7
            glm::vec2 size;      // 8..15
            float     rotation;  // 16..19
            float     zSortKey;  // 20..23
            uint32_t  slot;      // 24..27
            uint32_t  flags;     // 28..31
            uint32_t  _pad0;     // 32..35
            uint32_t  _pad1;     // 36..39

            alignas(8) glm::uvec2 uvMin16;  // 40..47
            glm::uvec2            uvMax16;  // 48..55
        };

        static_assert(offsetof(RenderInstance, uvMin16) == 40);
        static_assert(sizeof(RenderInstance) == 56);



    public:
        static constexpr uint32_t FRAMES_IN_FLIGHT = MAX_FRAMES_IN_FLIGHT;
        static constexpr uint32_t MAX_RESIDENT = MAX_RESIDENT_LAYERS; // keep under per-stage limits

        VulkanBindlessDescriptorSetRenderer(VkDevice device, bool updateAfterBindSupported);
        ~VulkanBindlessDescriptorSetRenderer();

        void Init(VkDevice device, bool updateAfterBindSupported);
        void Shutdown(VkDevice device);


        void BeginFrame(uint32_t frameIndex);
        void AddSpriteInstance(glm::vec2 worldCenter, float zKey, uint32_t spriteSlot, glm::uvec2 uvMin16, glm::uvec2 uvMax16, glm::vec2 sizeWorld, float rotation);
        void AddInstance(glm::vec2 worldPos, float zSortKey, uint32_t slot, float rotation, uint32_t flags = 0);
        void EndFrameAndUpload(uint32_t frameIndex);

        void Upload(uint32_t frameIndex);

        // Call once after swapchain/device init
        void SetTileDimensions(uint32_t w, uint32_t h) { m_tileW = w; m_tileH = h; }
        void SetAtlasExtent(VkExtent3D e) { m_atlasExtent = e; }

        // Per-frame
 
        void UpdateEffectImageDescriptorSets(size_t frameIndex, const std::array<Ref<VulkanTexture>, CHUNK_GRID_WIDTH* CHUNK_GRID_WIDTH>& textures);

        void UpdateLightBufferDescriptor(uint32_t frameIndex);
        void UpdateShadowMapDescriptorSets(Ref<VulkanShadowMap> shadowmap);


        void SetCurrentFrameIndex(uint32_t fi) { m_currentFrame = fi; }
        void EvictAllTiles();

        // Build visible instances and stream into SSBO; updates binding 2 for this frame
        void RecordTiles(VkCommandBuffer cmd, uint32_t frameIndex, const glm::mat4& VP, VkExtent2D fbExtent, const glm::mat4& lightMat);
        void DrawTilesShadowPass(VkCommandBuffer cmd, uint32_t frameIndex, VkPipeline shadowPipeline, VkPipelineLayout shadowPipelineLayout, const glm::mat4& lightSpaceMatrix, const glm::vec3& lightDir);
        uint32_t EnsureTileResident(uint64_t uid, const glm::vec4& atlasUV, VkCommandBuffer uploadCmd);
        uint32_t EnsureTileResidentFromRaw(uint64_t uid, const uint8_t* colorData, size_t colorSize, const uint8_t* propsData, size_t propsSize, VkCommandBuffer uploadCB);
        void ComputeBindBuffers(uint32_t frameIndex, VkBuffer resultsBuf, VkDeviceSize resultsSize, VkBuffer projectilesBuf, VkDeviceSize projSize, VkBuffer blockedMaskBuf, VkDeviceSize maskSize);

        void EffectsBindBuffers(uint32_t frameIndex, VkBuffer resultsBuf, VkDeviceSize resultsSize, VkBuffer projectilesBuf, VkDeviceSize projSize, VkBuffer blockedMaskBuf, VkDeviceSize maskSize);

        void UpdateCollisionResultDescriptor(uint32_t frameIndex, VkDescriptorSet dstSet, VkBuffer resultsBuf);


        //spritesheet
        bool AcquireSpritesheet(const std::string& path, uint32_t& outSlot, uint32_t& outW, uint32_t& outH);
        void ReleaseSpritesheet(uint32_t slot);


        // Accessors for your render code
        VkDescriptorSetLayout GetSetLayout() const { return m_bindlessSetLayout; }
        VkPipelineLayout      GetPipelineLayout() const { return m_tilesPipelineLayout; }
        VkPipelineLayout      GetComputePipelineLayout() const { return m_computePipelineLayout; }
        VkPipelineLayout      GetEffectsPipelineLayout() const { return m_effectsPipelineLayout; }

        VkPipeline            GetComputePipeline() const   { return m_computePipeline; }
        VkPipeline            GetEffectsPipeline() const { return m_effectsPipeline; }

        VkDescriptorSet       GetEffectsDescriptorSet(size_t frameIndex) { return m_effectsDescriptorSet[frameIndex]; }
        VkDescriptorSet       GetSetForFrame(uint32_t f) const { return m_bindlessSet[f]; }
        VkDescriptorSet       GetComputeDescriptorSetFrame(uint32_t f) const { return m_computeDescriptorSet[f]; }
        VkSampler             GetTileSampler() const { return m_tileSampler; }
        uint32_t              GetDrawCount() const { return m_drawCount; }
        VkBuffer              GetInstanceBuffer(uint32_t f) const { return m_instanceBuffer.buf[f]; }
        VkImage               GetColorImageArray() const { return      m_colorArrayImage; }
        VkImage               GetPropsArrayImage() const { return      m_propsArrayImage; }

        VkImageView         GetColorImageView(uint32_t slot) const  { return m_colorLayerPool.View(slot); }
        VkImageView         GetCPropsImageView(uint32_t slot) const  { return m_propsLayerPool.View(slot); }

        std::unordered_map<uint64_t, uint32_t>& GetTileToSlotMap() { return  m_tileToSlot; }

    private:
        // ----- internal helpers
        void CreateTileSampler(VkDevice device);
        void CreateBindlessSetLayout(VkDevice device, bool updateAfterBindSupported);
        void CreateTilesPipeline(VkDevice device, VkRenderPass renderPass);
        void CreateBindlessPoolAndSet(VkDevice device, bool updateAfterBindSupported);
        void CreateTilesPipelineLayout(VkDevice device);
        void CreateInstanceBuffers(VkDevice dev, VkPhysicalDevice phys, InstanceBuffer& out, size_t maxInstances);
        void CreateComputeDescriptorSet(VkDescriptorPool computeDescriptorPool);
        void CreateComputeArrayDescriptorSetLayout(uint32_t maxResidentLayers, bool updateAfterBindSupported);
        void CreateComputeGraphicsPipeline();

        void CreateEffectsDescriptorSetLayout();
        void CreateEffectsPipeline();
        void CreateEffectsPipelineLayout();
        void CreateEffectsDescriptorSets(VkDescriptorPool computeDescriptorPool);

        void CreateColorArray(VkDevice dev, VkPhysicalDevice phys); // one 2D array image, per-layer views

        void CreatePropsArray(VkDevice dev, VkPhysicalDevice phys);

        void ComputeWriteImageSlot(uint32_t frameIndex, uint32_t arrayIndex, VkImageView colorView, VkImageView propsView);

        void CopyFromAtlasUVToLayer(VkCommandBuffer cmd, VkImage atlas, const glm::vec4& uv,
            VkImage dstArray, uint32_t layer, uint32_t tileW, uint32_t tileH);

        void WriteInstanceBufferToDescriptor(VkDevice dev, VkDescriptorSet set, VkBuffer buf);

        // Descriptor write helpers (write one array element)
        void WriteCombinedImageSampler(VkDevice device, VkDescriptorSet set, uint32_t binding,
            uint32_t arrayIndex, VkSampler sampler, VkImageView view,
            VkImageLayout layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
        void WriteStorageImage(VkDevice device, VkDescriptorSet set, uint32_t binding,
            uint32_t arrayIndex, VkImageView view,
            VkImageLayout layout = VK_IMAGE_LAYOUT_GENERAL);

        // Barriers (simple versions)
        void TransitionImage(VkCommandBuffer cmd, VkImage img,
            VkImageLayout oldLayout, VkImageLayout newLayout,
            VkImageSubresourceRange range);
        void TransitionImageLayer(VkCommandBuffer cmd, VkImage img,
            VkImageLayout oldLayout, VkImageLayout newLayout,
            uint32_t layer);





        void UploadToArrayLayerViaStaging_ST(const void* srcData, size_t numBytes, VkImage dstImage, VkImageLayout currentLayout, VkImageLayout finalLayout, uint32_t layer, uint32_t width, uint32_t height);

        // Simple helpers you can replace with your own
        bool IsInsideView(const Camera& cam, glm::vec2 worldPos) const; // TODO: implement properly
    

        //spritesheet
        uint32_t AllocSpriteSlot_();
        void     WriteSpriteDescriptorAllFrames_(uint32_t slot, VkImageView view);

    private:
        // Device references
        VkDevice m_device = VK_NULL_HANDLE;

        // Descriptor stuff
        VkDescriptorPool      m_descPool = VK_NULL_HANDLE;   // Vulkan pool
        std::array<VkDescriptorSet, FRAMES_IN_FLIGHT> m_bindlessSet{};
        std::array<VkDescriptorSet, FRAMES_IN_FLIGHT> m_computeDescriptorSet;
        std::array<VkDescriptorSet, FRAMES_IN_FLIGHT> m_effectsDescriptorSet;

        VkDescriptorSetLayout m_effectsDescriptorSetLayout; // remove?
        VkDescriptorSetLayout m_bindlessSetLayout = VK_NULL_HANDLE;
        VkDescriptorSetLayout m_computeDescriptorSetLayout;


        VkPipelineLayout      m_computePipelineLayout;
        VkPipelineLayout      m_tilesPipelineLayout = VK_NULL_HANDLE;
        VkPipelineLayout      m_effectsPipelineLayout;

        VkPipeline            m_effectsPipeline;
        VkPipeline            m_computePipeline;
        VkPipeline            m_tilesPipeline = VK_NULL_HANDLE;
        VkSampler             m_tileSampler = VK_NULL_HANDLE;

        Ref<VulkanShader> m_computeShader;
        Ref<VulkanShader> m_effectShader;

        // Shaders / pipeline (you instantiate your pipeline elsewhere)
        std::shared_ptr<VulkanShader> m_bindlessDescriptorsShader;

        // Instance data
        InstanceBuffer m_instanceBuffer{};
        std::vector<RenderInstance> m_instances;
        uint32_t m_drawCount = 0;

        // Upload cmd for this frame (single-use CB you begin/submit before drawing)
        VkCommandBuffer m_uploadCmdThisFrame = VK_NULL_HANDLE;
        uint32_t        m_currentFrame = 0;
        uint32_t        m_arrayLayerCount = 0;
        // Atlas info
        VkExtent3D m_atlasExtent{ 0,0,1 };

        // Per-tile color array (one layer per resident tile)
        VkImage        m_colorArrayImage = VK_NULL_HANDLE;
        VkImage        m_propsArrayImage = VK_NULL_HANDLE;
        VkDeviceMemory m_colorArrayMem = VK_NULL_HANDLE;
        VkDeviceMemory m_propsArrayMem = VK_NULL_HANDLE;
        uint32_t       m_tileW = TILE_PIXEL_WIDTH;
        uint32_t       m_tileH = TILE_PIXEL_HEIGHT;

        // Per-layer views + freelist
        

        LayerPool m_colorLayerPool;
        LayerPool m_propsLayerPool;

        // Residency map: tile UID -> slot (layer)
        std::unordered_map<uint64_t, uint32_t> m_tileToSlot;


        //spritesheet
        std::unordered_map<std::string, SpriteRec> m_spriteByPath;
        std::unordered_map<uint32_t, std::string>  m_spritePathBySlot;
        std::vector<uint32_t>                      m_freeSpriteSlots;
        uint32_t                                   m_nextSpriteSlot = 0;
    };

} 
