#include "pch.h"
#include "VulkanLighting.h"
#include <Engine/Core/Assert.h>
#include "GPULightBuffer.h"
#include <Engine/Platform/Vulkan/VulkanBuffer.h>
#include "Engine/Renderer/Lights/LightSubmitFrame.h"


namespace Engine {

    //lights
    Ref<VulkanBuffer> VulkanLighting::s_gpuLightBuffer;
    Ref<LightSubmitFrame> VulkanLighting::s_lightSubmitData;


    static inline glm::vec3 NormalizeSafe(const glm::vec3& v, const glm::vec3& fallback)
    {
        const float len2 = glm::dot(v, v);
        if (len2 > 1e-8f)
        {
            return v * (1.0f / std::sqrt(len2));

        }
        return fallback;
    }


    void VulkanLighting::Init(VkDevice device, VkPhysicalDevice physicalDevice)
    {
        s_lightSubmitData = std::make_shared<LightSubmitFrame>();


    }



    void VulkanLighting::BeginFrame(uint32_t frameIndex)
    {
        auto& f = s_lightSubmitData;
        f->points.clear();
        f->spots.clear();
        f->dirs.clear();
    }

    void VulkanLighting::SubmitDirectional(const glm::vec3& dirWS, const glm::vec3& color, float intensity)
    {
        Ref<LightSubmitFrame>& f = s_lightSubmitData;



        // Keep only one directional (sun) by default
        if (f->dirs.size() >= MAX_DIR_LIGHTS)
            return;

        GPUDirectionalLight L{};
       
        glm::vec3 d = NormalizeSafe(dirWS, glm::vec3(0.0f, -1.0f, 0.0f));
        L.direction_intensity = glm::vec4(d, std::max(0.0f, intensity));
        L.color = glm::vec4(glm::max(color, glm::vec3(0.0f)), 0.0f);

        f->dirs.push_back(L);
    }

    void VulkanLighting::SubmitPoint(const glm::vec3& posWS, const glm::vec3& color, float intensity, float radius)
    {
        Ref<LightSubmitFrame>& f = s_lightSubmitData;

        if (f->points.size() >= MAX_POINT_LIGHTS)
            return;

        GPUPointLight L{};
        L.position_radius = glm::vec4(posWS, std::max(0.0f, radius));
        L.color_intensity = glm::vec4(glm::max(color, glm::vec3(0.0f)), std::max(0.0f, intensity));

        f->points.push_back(L);
    }

    void VulkanLighting::SubmitSpot(const glm::vec3& posWS, const glm::vec3& dirWS, const glm::vec3& color,
        float intensity, float range, float innerAngleRad, float outerAngleRad)
    {
        Ref<LightSubmitFrame>& f = s_lightSubmitData;

        if (f->spots.size() >= MAX_SPOT_LIGHTS)
            return;

        // Ensure sane ordering: inner <= outer
        const float innerA = std::max(0.0f, std::min(innerAngleRad, outerAngleRad));
        const float outerA = std::max(innerA, outerAngleRad);

        GPUSpotLight L{};
        glm::vec3 d = NormalizeSafe(dirWS, glm::vec3(0.0f, -1.0f, 0.0f));

        L.position_range = glm::vec4(posWS, std::max(0.0f, range));
        L.direction_inner = glm::vec4(d, std::cos(innerA));
        L.color_outer = glm::vec4(glm::max(color, glm::vec3(0.0f)), std::cos(outerA));
        L.intensity_pad = glm::vec4(std::max(0.0f, intensity), 0.0f, 0.0f, 0.0f);

        f->spots.push_back(L);
    }

    void VulkanLighting::FlushAndUpload(VkCommandBuffer /*cmd*/, uint32_t frameIndex)
    {
        const uint32_t fi = 0;
        const auto& f = s_lightSubmitData;

        GPULightBuffer gpu{};
        gpu.header.numDir = (uint32_t)std::min<size_t>(f->dirs.size(), MAX_DIR_LIGHTS);
        gpu.header.numPoint = (uint32_t)std::min<size_t>(f->points.size(), MAX_POINT_LIGHTS);
        gpu.header.numSpot = (uint32_t)std::min<size_t>(f->spots.size(), MAX_SPOT_LIGHTS);
        gpu.header.pad = 0;

        for (uint32_t i = 0; i < gpu.header.numDir; ++i)
        {

            gpu.dir[i] = f->dirs[i];
        }

        for (uint32_t i = 0; i < gpu.header.numPoint; ++i)
        {

            gpu.point[i] = f->points[i];
        }

        for (uint32_t i = 0; i < gpu.header.numSpot; ++i)
        {

            gpu.spot[i] = f->spots[i];
        }

        void* mapped = s_gpuLightBuffer->Mapped();
        std::memcpy(mapped, &gpu, sizeof(GPULightBuffer));
    }
}