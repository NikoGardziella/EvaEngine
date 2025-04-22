#pragma once

#include "Engine/Platform/Vulkan/VulkanTexture.h"
#include <vector>

namespace Engine {

    class VulkanPixelTexture : public VulkanTexture
    {
    public:
        VulkanPixelTexture(uint32_t width, uint32_t height);

        void SetPixel(uint32_t x, uint32_t y, uint8_t r, uint8_t g, uint8_t b, uint8_t a);
        void ApplyChanges(); // Upload to GPU

    private:
        std::vector<uint8_t> m_pixelData; // 4 bytes per pixel (RGBA)
    };

}