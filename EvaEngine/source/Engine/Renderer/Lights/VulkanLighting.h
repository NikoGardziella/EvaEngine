#pragma once
#include "LightSubmitFrame.h"
#include "vulkan/vulkan.h"
#include <Engine/Platform/Vulkan/VulkanBuffer.h>
#include <Engine/Core/Core.h>


namespace Engine {


    class VulkanLighting
    {
    public:
        static void BeginFrame(uint32_t frameIndex);

        void Init(VkDevice device, VkPhysicalDevice physicalDevice);

        static void SubmitDirectional(const glm::vec3& dirWS, const glm::vec3& color, float intensity);
        static void SubmitPoint(const glm::vec3& posWS, const glm::vec3& color, float intensity, float radius);
        static void SubmitSpot(const glm::vec3& posWS, const glm::vec3& dirWS, const glm::vec3& color,
            float intensity, float range, float innerAngleRad, float outerAngleRad);

        static void FlushAndUpload(VkCommandBuffer cmd, uint32_t frameIndex);

        static Ref<VulkanBuffer>& GetLightBuffer()
        {
            return s_gpuLightBuffer;
        }

        static Ref<LightSubmitFrame>& GetLightSubmitFrameData()
        {
            return s_lightSubmitData;
        }


    private:
     
    private:
        static Ref<VulkanBuffer> s_gpuLightBuffer;

        static Ref<LightSubmitFrame> s_lightSubmitData;

  

    };

}
