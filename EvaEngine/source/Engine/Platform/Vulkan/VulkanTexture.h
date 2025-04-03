#pragma once

#include "Engine/Renderer/Texture.h"
#include "vulkan/vulkan.h"
#include <string>

namespace Engine {

    class VulkanTexture : public Texture2D
    {
    public:
        VulkanTexture(const std::string& path);
        VulkanTexture(uint32_t width, uint32_t height);

        virtual ~VulkanTexture();

        virtual uint32_t GetWidth() const override { return m_width; }
        virtual uint32_t GetHeight() const override { return m_height; }
        virtual void Bind(uint32_t slot = 0) const override;
       virtual uint32_t GetRendererID() const override { return 0; }

        virtual void SetData(void* data, uint32_t size) override;

        virtual bool operator==(const Texture& other) const override
        {
            const VulkanTexture* vulkanOther = dynamic_cast<const VulkanTexture*>(&other);
            if (!vulkanOther)
                return false;
            return m_imageView == vulkanOther->m_imageView;
        }

    private:
        void CreateTextureImage(const std::string& path);
        void CreateTextureImageView();
        void CreateTextureSampler();

        std::string m_path;
        uint32_t m_width;
        uint32_t m_height;
        VkImage m_image;
        VkDeviceMemory m_imageMemory;
        VkImageView m_imageView;
        VkSampler m_sampler;
    };

}
