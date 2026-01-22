#pragma once

#include "Engine/Renderer/Texture.h"
#include "vulkan/vulkan.h"
#include <string>

#include "glm/glm.hpp"
#include <Engine/Animation/3D/Import/GLTFImporter.h>
#include <Engine/Core/Core.h>

namespace Engine {

    class VulkanTexture : public Texture2D
    {
    public:
        VulkanTexture(const std::string& path, VkFormat textureFormat = VK_FORMAT_R8G8B8A8_UNORM, const std::string& name = "",bool imGuiTexture = false, uint32_t textureID = 0);
        VulkanTexture(uint32_t width, uint32_t height, VkFormat textureFormat = VK_FORMAT_R8G8B8A8_UNORM, bool imGuiTexture = false, uint32_t textureID = 0);
        VulkanTexture(const TextureSource& src);
        
        virtual ~VulkanTexture();

        virtual uint32_t GetWidth() const override { return m_width; }
        void SetWidth(uint32_t width) { m_width = width; }
        virtual uint32_t GetHeight() const override { return m_height; }
        void SetHeight(uint32_t height) { m_height = height; }
        VkExtent3D GetExtent() const { return { m_width, m_height, 1 }; }
        VkExtent2D GetExtent2D() const { return { m_width, m_height }; }

        virtual void Bind(uint32_t slot = 0) const override;
        virtual uint32_t GetRendererID() const override { return 0; }
		VkImageView GetImageView() const { return m_imageView; }
        VkImage GetImage() const { return m_image;  }

     

		VkSampler GetSampler() const { return m_sampler; }
		VkDescriptorSet GetTextureDescriptor() const { return m_textureDescriptor; }
		const std::string GetPath() const { return m_path; }

		const std::string& GetName() const { return m_name; }
		void SetName(std::string name) { m_name = name; }

		
        uint32_t GetTextureID() const { return m_TextureID; }
        void SetTextureID(uint32_t textureID) { m_TextureID = textureID; }

        virtual void SetData(void* data, uint32_t size) override;
        void ResetData() const;
        void CreateTextureImage(VkImageUsageFlags usage);
        Ref<VulkanTexture> Clone() const;

        const std::vector<uint8_t>& GetCPUPixelData() const { return m_CPUpixelData; }
         std::vector<uint8_t>& GetCPUPixelData()  { return m_CPUpixelData; }
		void SetCPUPixelData(const std::vector<uint8_t>& pixelData) { m_CPUpixelData = pixelData; }

        void SetCheckCollision(bool checkCollision) { m_checkCollision = checkCollision; }
        bool GetCheckCollision() const { return m_checkCollision; }

		void SetTextureOrigin(const glm::vec2& origin) { m_texureOrigin = origin; }
        glm::vec2 GetTextureOrigin() { return m_texureOrigin; }

		void SetPixelSize(float pixelSize) { m_pixelSize = pixelSize; }
		float GetPixelSize() const { return m_pixelSize; }

        VkImageLayout GetCurrentLayout() const { return m_CurrentLayout; }
        void SetCurrentLayout(VkImageLayout layout) { m_CurrentLayout = layout; }


        void CreateTextureImage(const std::string& path);
        void CreateTextureImageView();
        void CreateTextureSampler();
        void CopyFrom(const VulkanTexture& src);



        //****************************

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
        std::vector<uint8_t> m_CPUpixelData; // 4 bytes per pixel (RGBA)
    private:
        

        std::string m_path;
        std::string m_name;
        VkDeviceMemory m_imageMemory;
        VkImageView m_imageView;
        VkDescriptorSet m_textureDescriptor;
        VkDeviceSize m_memorySize = 0;
        uint32_t m_TextureID;
        VkSampler m_sampler;
        VkFormat m_textureFormat = VK_FORMAT_R8G8B8A8_UNORM;

        VkImageLayout m_CurrentLayout = VK_IMAGE_LAYOUT_UNDEFINED;

        bool m_checkCollision = false;
        glm::vec2 m_texureOrigin;
		float m_pixelSize = 1.0f;

    };

}
