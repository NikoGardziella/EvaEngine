#include "pch.h"
#include "VulkanShader.h"
#include "Engine/AssetManager/AssetManager.h"
#include "Engine/Platform/Vulkan/VulkanContext.h"

#include <shaderc/shaderc.hpp>
#include "Engine/Debug/Instrumentor.h"

namespace Engine {

    namespace Utils {

        static const char* VulkanShaderStageCachedFileExtension(VkShaderStageFlagBits stage)
        {
            switch (stage)
            {
            case VK_SHADER_STAGE_VERTEX_BIT:    return ".vert";
            case VK_SHADER_STAGE_FRAGMENT_BIT:  return ".frag";
            default:
                EE_CORE_ASSERT(false, "Unsupported shader stage!");
                return "";
            }
        }

    }

    VulkanShader::VulkanShader(const std::string& filepath)
    {
		m_device = VulkanContext::Get()->GetDeviceManager().GetDevice();
		m_Name = std::filesystem::path(filepath).stem().string();
        std::string source = ReadFile(filepath);
        auto shaderSources = PreProcess(source);
        CompileOrGetVulkanBinaries(shaderSources, filepath);
    }

    VulkanShader::VulkanShader(const std::string& name, const std::string& vertexSource, const std::string& fragmentSource)
        : m_Name(name)
    {
        std::unordered_map<VkShaderStageFlagBits, std::string> shaderSources;
        shaderSources[VK_SHADER_STAGE_VERTEX_BIT] = vertexSource;
        shaderSources[VK_SHADER_STAGE_FRAGMENT_BIT] = fragmentSource;
        CompileOrGetVulkanBinaries(shaderSources, name);
    }

    VulkanShader::~VulkanShader()
    {
        vkDestroyShaderModule(m_device, m_VertexShaderModule, nullptr);
        vkDestroyShaderModule(m_device, m_FragmentShaderModule, nullptr);
    }

    void VulkanShader::Bind() const
    {
        // Binding logic for Vulkan shaders
    }

    void VulkanShader::Unbind() const
    {
        // Unbinding logic for Vulkan shaders
    }

    void VulkanShader::SetMat4(const std::string& name, const glm::mat4& value)
    {
        // Set uniform logic for mat4
    }

    void VulkanShader::SetFloat4(const std::string& name, const glm::vec4& value)
    {
        // Set uniform logic for vec4
    }

    void VulkanShader::SetFloat3(const std::string& name, const glm::vec3& value)
    {
        // Set uniform logic for vec3
    }

    void VulkanShader::SetFloat2(const std::string& name, const glm::vec2& value)
    {
        // Set uniform logic for vec2
    }

    void VulkanShader::SetFloat(const std::string& name, float value)
    {
        // Set uniform logic for float
    }

    void VulkanShader::SetInt(const std::string& name, int value)
    {
        // Set uniform logic for int
    }

    void VulkanShader::SetIntArray(const std::string& name, int* values, uint32_t count)
    {
        // Set uniform logic for int array
    }

    void VulkanShader::CompileOrGetVulkanBinaries(const std::unordered_map<VkShaderStageFlagBits, std::string>& shaderSources, const std::string& originalFilePath)
    {
        shaderc::Compiler compiler;
        shaderc::CompileOptions options;
        options.SetTargetEnvironment(shaderc_target_env_vulkan, shaderc_env_version_vulkan_1_2);
        options.SetTargetSpirv(shaderc_spirv_version_1_0);

        const bool optimize = true;
        if (optimize)
        {
            options.SetOptimizationLevel(shaderc_optimization_level_performance);
        }

        std::filesystem::path cacheDirectory = AssetManager::GetVulkanCacheDirectory();

        if (!std::filesystem::exists(cacheDirectory))
        {
            std::filesystem::create_directories(cacheDirectory);
        }

        for (auto& kv : shaderSources)
        {
            shaderc_shader_kind kind;
            switch (kv.first)
            {
            case VK_SHADER_STAGE_VERTEX_BIT:
                kind = shaderc_glsl_vertex_shader;
                break;
            case VK_SHADER_STAGE_FRAGMENT_BIT:
                kind = shaderc_glsl_fragment_shader;
                break;
            default:
                throw std::runtime_error("Unsupported shader stage");
            }

            std::filesystem::path shaderFilePath = std::filesystem::path(originalFilePath).filename();
            std::filesystem::path cachedPath = cacheDirectory / (shaderFilePath.string() + Utils::VulkanShaderStageCachedFileExtension(kv.first));

            std::vector<uint32_t> spirv;
            std::ifstream in(cachedPath, std::ios::in | std::ios::binary);
            if (in.is_open())
            {
                in.seekg(0, std::ios::end);
                auto size = in.tellg();
                in.seekg(0, std::ios::beg);
                spirv.resize(size / sizeof(uint32_t));
                in.read((char*)spirv.data(), size);
            }
            else
            {
                InstrumentationTimer timer("Compile shader");
                shaderc::SpvCompilationResult module = compiler.CompileGlslToSpv(kv.second, kind, m_Name.c_str(), options);
                if (module.GetCompilationStatus() != shaderc_compilation_status_success)
                {
                    std::cerr << "Shader compilation error: " << module.GetErrorMessage() << std::endl;
                    throw std::runtime_error(module.GetErrorMessage());
                }

                spirv = std::vector<uint32_t>(module.cbegin(), module.cend());

                std::ofstream out(cachedPath, std::ios::out | std::ios::binary);
                if (out.is_open())
                {
                    out.write((char*)spirv.data(), spirv.size() * sizeof(uint32_t));
                    out.flush();
                    out.close();
                }
                timer.Stop();
                EE_CORE_WARN("Shader compilation took {} ms", timer.GetElapsedTime().count());
            }

            VkShaderModule shaderModule;
            CreateShaderModule(spirv, &shaderModule);

            if (kv.first == VK_SHADER_STAGE_VERTEX_BIT)
            {
                m_VertexShaderModule = shaderModule;
            }
            else if (kv.first == VK_SHADER_STAGE_FRAGMENT_BIT)
            {
                m_FragmentShaderModule = shaderModule;
            }
        }
    }

    void VulkanShader::CreateShaderModule(const std::vector<uint32_t>& code, VkShaderModule* shaderModule)
    {
        VkShaderModuleCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        createInfo.codeSize = code.size() * sizeof(uint32_t);
        createInfo.pCode = code.data();

        if (vkCreateShaderModule(m_device, &createInfo, nullptr, shaderModule) != VK_SUCCESS)
        {
            throw std::runtime_error("failed to create shader module!");
        }
    }

    std::string VulkanShader::ReadFile(const std::string& filepath)
    {
        std::cout << "Reading shader file: " << filepath << std::endl;

        std::ifstream file(filepath, std::ios::ate | std::ios::binary);

        if (!file.is_open()) {
            std::cerr << "Failed to open file: " << filepath << std::endl;
            throw std::runtime_error("failed to open file!");
        }

        size_t fileSize = (size_t)file.tellg();
        std::string buffer(fileSize, ' ');

        file.seekg(0);
        file.read(&buffer[0], fileSize);

        file.close();
        return buffer;
    }

    std::unordered_map<VkShaderStageFlagBits, std::string> VulkanShader::PreProcess(const std::string& source)
    {
        std::unordered_map<VkShaderStageFlagBits, std::string> shaderSources;

        const char* typeToken = "#type";
        size_t typeTokenLength = strlen(typeToken);
        size_t pos = source.find(typeToken, 0);
        while (pos != std::string::npos)
        {
            size_t eol = source.find_first_of("\r\n", pos);
            size_t begin = pos + typeTokenLength + 1;
            std::string type = source.substr(begin, eol - begin);

            size_t nextLinePos = source.find_first_not_of("\r\n", eol);
            pos = source.find(typeToken, nextLinePos);
            shaderSources[ShaderTypeFromString(type)] = source.substr(nextLinePos, pos - nextLinePos);
        }

        return shaderSources;
    }

    VkShaderStageFlagBits VulkanShader::ShaderTypeFromString(const std::string& type)
    {
        if (type == "vertex")
            return VK_SHADER_STAGE_VERTEX_BIT;
        if (type == "fragment" || type == "pixel")
            return VK_SHADER_STAGE_FRAGMENT_BIT;

        throw std::runtime_error("Unknown shader type!");
    }

}