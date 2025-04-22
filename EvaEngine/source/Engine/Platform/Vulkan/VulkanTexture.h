#pragma once

#include "Engine/Renderer/Texture.h"
#include "vulkan/vulkan.h"
#include <string>

namespace Engine {

    class VulkanTexture : public Texture2D
    {
    public:
        VulkanTexture(const std::string& path, bool imGuiTexture = false);
        VulkanTexture(uint32_t width, uint32_t height);

        virtual ~VulkanTexture();

        virtual uint32_t GetWidth() const override { return m_width; }
        virtual uint32_t GetHeight() const override { return m_height; }
        virtual void Bind(uint32_t slot = 0) const override;
        virtual uint32_t GetRendererID() const override { return 0; }
		VkImageView GetImageView() const { return m_imageView; }
		VkSampler GetSampler() const { return m_sampler; }
		VkDescriptorSet GetTextureDescriptor() const { return m_textureDescriptor; }

        virtual void SetData(void* data, uint32_t size) override;

        virtual bool operator==(const Texture& other) const override
        {
            const VulkanTexture* vulkanOther = dynamic_cast<const VulkanTexture*>(&other);
            if (!vulkanOther)
                return false;
            return m_imageView == vulkanOther->m_imageView;
        }

    protected:
        VkImage m_image;
        uint32_t m_width;
        uint32_t m_height;
        std::vector<uint8_t> m_pixelData; // 4 bytes per pixel (RGBA)
    private:
        void CreateTextureImage(const std::string& path);
        void CreateTextureImageView();
        void CreateTextureSampler();

        std::string m_path;
        VkDeviceMemory m_imageMemory;
        VkImageView m_imageView;
        VkDescriptorSet m_textureDescriptor;
        VkDeviceSize m_memorySize = 0;

        VkSampler m_sampler;
    };

}
