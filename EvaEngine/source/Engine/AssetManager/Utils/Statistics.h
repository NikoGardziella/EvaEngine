#pragma once
#include <vulkan/vulkan_core.h>

namespace Engine {


    class GPUStats
    {
    public:

        static GPUStats & Get()
        {
            static GPUStats s_instance;
            return s_instance;
        }

        GPUStats(const GPUStats&) = delete;
        GPUStats& operator=(const GPUStats&) = delete;
        GPUStats(GPUStats&&) = delete;
        GPUStats& operator=(GPUStats&&) = delete;


        void AddTexture(VkDeviceSize bytes) { m_textures += bytes; }
        void AddImage(VkDeviceSize bytes) { m_images += bytes; }
        void AddBuffer(VkDeviceSize bytes) { m_buffers += bytes; }

        void RemoveTexture(VkDeviceSize bytes) { m_textures -= bytes; }
        void RemoveImage(VkDeviceSize bytes) { m_images -= bytes; }
        void RemoveBuffer(VkDeviceSize bytes) { m_buffers -= bytes; }

        VkDeviceSize GetTextures()  const { return m_textures; }
        VkDeviceSize GetImages()    const { return m_images; }
        VkDeviceSize GetBuffers()   const { return m_buffers; }

    private:
        GPUStats() = default;
        ~GPUStats() = default;

        VkDeviceSize m_textures = 0; // disk textures / atlases
        VkDeviceSize m_images = 0; // render targets / chunk images / depth
        VkDeviceSize m_buffers = 0; // SSBO / UBO / vertex / index
    };
}