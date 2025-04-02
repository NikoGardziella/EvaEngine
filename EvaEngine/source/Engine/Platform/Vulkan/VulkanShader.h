#pragma once

#include "Engine/Renderer/Shader.h"
#include <vulkan/vulkan.h>
#include <glm/glm.hpp>
#include <unordered_map>
#include <string>
#include <vector>

namespace Engine {

    class VulkanShader : public Shader
    {
    public:
        VulkanShader(const std::string& filepath);
        VulkanShader(const std::string& name, const std::string& vertexSource, const std::string& fragmentSource);
        virtual ~VulkanShader();

        virtual void Bind() const override;
        virtual void Unbind() const override;

        virtual void SetMat4(const std::string& name, const glm::mat4& value) override;
        virtual void SetFloat4(const std::string& name, const glm::vec4& value) override;
        virtual void SetFloat3(const std::string& name, const glm::vec3& value) override;
        virtual void SetFloat2(const std::string& name, const glm::vec2& value) override;
        virtual void SetFloat(const std::string& name, float value) override;
        virtual void SetInt(const std::string& name, int value) override;
        virtual void SetIntArray(const std::string& name, int* values, uint32_t count) override;


        virtual const std::string& GetName() const override { return m_Name; }
		VkShaderModule GetVertexShaderModule() const { return m_VertexShaderModule; }
		VkShaderModule GetFragmentShaderModule() const { return m_FragmentShaderModule; }
    private:

        std::string ReadFile(const std::string& filepath);
        std::unordered_map<VkShaderStageFlagBits, std::string> PreProcess(const std::string& source);

        VkShaderStageFlagBits ShaderTypeFromString(const std::string& type);

        void CompileOrGetVulkanBinaries(const std::unordered_map<VkShaderStageFlagBits, std::string>& shaderSources, const std::string& originalFilePath);

        void CreateShaderModule(const std::vector<uint32_t>& code, VkShaderModule* shaderModule);

    private:
        std::string m_Name;
        VkDevice m_device;
        VkPipelineShaderStageCreateInfo m_ShaderStages[2];
        VkShaderModule m_VertexShaderModule;
        VkShaderModule m_FragmentShaderModule;
        std::unordered_map<std::string, VkDescriptorSetLayoutBinding> m_UniformLocations;

    };

}
