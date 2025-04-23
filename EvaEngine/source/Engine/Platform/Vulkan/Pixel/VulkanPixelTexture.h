#pragma once

#include "Engine/Platform/Vulkan/VulkanTexture.h"
#include <vector>

namespace Engine {

    class VulkanPixelTexture : public VulkanTexture
    {
    public:
        VulkanPixelTexture(const std::string& path);

        void SetPixel(uint32_t x, uint32_t y, uint8_t r, uint8_t g, uint8_t b, uint8_t a);
        void ApplyChanges(); // Upload to GPU

        bool IsPixelSolid(int x, int y) const;

        void DestroyPixel(int x, int y);

    private:
        std::vector<uint8_t> m_solidMask;
    };

}