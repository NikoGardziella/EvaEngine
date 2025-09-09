#pragma once
#include <vulkan/vulkan.h>
#include <vector>
#include <array>
#include <unordered_map>
#include <memory>
#include <glm/glm.hpp>
#include <entt.hpp>
#include <Engine/Scene/Components/Render/TileComponent.h>

namespace Engine {

    struct Camera;
    
    // --------- Render instance (std430-friendly, 32 bytes)
    struct RenderInstance {
        glm::vec2 worldPos;
        glm::vec2 size;
        float     zSortKey;
        uint32_t  slot;   // layer index in color array
        uint32_t  flags;
        uint32_t  _pad;
    };
    static_assert(sizeof(RenderInstance) == 32, "RenderInstance must be 32 bytes");

    // --------- Instance buffer triple-buffered
    struct InstanceBuffer {
        VkBuffer        buf[3]{};
        VkDeviceMemory  mem[3]{};
        void* mapped[3]{};
        VkDeviceSize    capacityBytes = 0;
    };

    class VulkanShader; // fwd

    class VulkanBindlessDescriptorSetRenderer {
    public:
        static constexpr uint32_t FRAMES_IN_FLIGHT = 3;
        static constexpr uint32_t MAX_RESIDENT = 2048; // keep under per-stage limits

        VulkanBindlessDescriptorSetRenderer();
        VulkanBindlessDescriptorSetRenderer(VkDevice device, bool updateAfterBindSupported);
        ~VulkanBindlessDescriptorSetRenderer();

        void Init(VkDevice device, bool updateAfterBindSupported);
        void Shutdown(VkDevice device);

       
        void BeginFrame(uint32_t frameIndex, VkCommandBuffer uploadCB);
        void AddInstance(glm::vec2 worldPos,float zSortKey, uint32_t slot, uint32_t flags = 0);
        void EndFrameAndUpload(uint32_t frameIndex);

        // Call once after swapchain/device init
        void SetTileDimensions(uint32_t w, uint32_t h) { m_tileW = w; m_tileH = h; }
        void SetAtlasExtent(VkExtent3D e) { m_atlasExtent = e; }

        // Per-frame
        void SetUploadCmdThisFrame(VkCommandBuffer cb) { m_uploadCmdThisFrame = cb; }
        void SetCurrentFrameIndex(uint32_t fi) { m_currentFrame = fi; }

        // Build visible instances and stream into SSBO; updates binding 2 for this frame
        void BuildInstancesFull(const Camera& cam, int frameIndex, entt::registry& registry);
        uint32_t EnsureTileResident(uint64_t uid, const glm::vec4& atlasUV, VkCommandBuffer uploadCB);
        void RecordTiles(VkCommandBuffer cmd, uint32_t frameIndex, const glm::mat4& VP, VkExtent2D fbExtent);

        // Accessors for your render code
        VkDescriptorSetLayout GetSetLayout() const { return m_bindlessSetLayout; }
        VkPipelineLayout      GetPipelineLayout() const { return m_tilesPipelineLayout; }
        VkDescriptorSet       GetSetForFrame(uint32_t f) const { return m_bindlessSet[f]; }
        VkSampler             GetTileSampler() const { return m_tileSampler; }
        uint32_t              GetDrawCount() const { return m_drawCount; }
        VkBuffer              GetInstanceBuffer(uint32_t f) const { return m_instanceBuffer.buf[f]; }

    private:
        // ----- internal helpers
        void CreateTileSampler(VkDevice device);
        void CreateBindlessSetLayout(VkDevice device, bool updateAfterBindSupported);
        void CreateTilesPipeline(VkDevice device, VkRenderPass renderPass);
        void CreateBindlessPoolAndSet(VkDevice device, bool updateAfterBindSupported);
        void CreateTilesPipelineLayout(VkDevice device);
        void CreateInstanceBuffers(VkDevice dev, VkPhysicalDevice phys, InstanceBuffer& out, size_t maxInstances);

        void CreateColorArray(VkDevice dev, VkPhysicalDevice phys); // one 2D array image, per-layer views

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

        // Simple helpers you can replace with your own
        bool IsInsideView(const Camera& cam, glm::vec2 worldPos) const; // TODO: implement properly
        void SubmitTile(uint64_t entID, entt::entity e, const glm::vec2& worldPosCenter, const glm::vec4& atlasUV, const std::string& name, const glm::vec2& localPos);
        float LayerBiasFor(const TileInfo& t) const
        {
            return  1.5f; 
        }

    private:
        // Device references
        VkDevice m_device = VK_NULL_HANDLE;

        // Descriptor stuff
        VkDescriptorSetLayout m_bindlessSetLayout = VK_NULL_HANDLE;
        VkDescriptorPool      m_descPool = VK_NULL_HANDLE;   // Vulkan pool
        std::array<VkDescriptorSet, FRAMES_IN_FLIGHT> m_bindlessSet{};
        VkPipelineLayout      m_tilesPipelineLayout = VK_NULL_HANDLE;
        VkPipeline            m_tilesPipeline = VK_NULL_HANDLE;
        VkSampler             m_tileSampler = VK_NULL_HANDLE;

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
        VkDeviceMemory m_colorArrayMem = VK_NULL_HANDLE;
        uint32_t       m_tileW = 128;
        uint32_t     m_tileH = 256; 

        // Per-layer views + freelist
        struct ColorLayerPool {
            VkDevice device{};
            VkImage  image{};
            VkFormat format{ VK_FORMAT_R8G8B8A8_UNORM };
            std::vector<uint32_t>    freeList;
            std::vector<VkImageView> views;
            void Init(VkDevice dev, VkImage img, uint32_t layerCount);
            void Shutdown(VkDevice dev);
            uint32_t Acquire();
            void Release(uint32_t i);
            VkImageView View(uint32_t i) const { return views[i]; }
        } m_colorLayerPool;

        // Residency map: tile UID -> slot (layer)
        std::unordered_map<uint64_t, uint32_t> m_tileToSlot;
    };

} // namespace Engine
