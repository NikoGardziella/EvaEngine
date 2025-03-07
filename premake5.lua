--include "Dependencies.lua"

-- Define the workspace, its name, and the architecture used
workspace "EvaEngine"
    architecture "x86_64"  -- Specify 64-bit architecture

    -- Define the available build configurations
    configurations
    {
        "Debug",     -- Configuration for development with debug symbols
        "Release",   -- Configuration for optimized build without debug symbols
        "Dist"       -- Configuration for distribution (final product) with high optimization
    }

-- Variable to determine the output directory format based on configuration, system, and architecture
outputdir = "%{cfg.buildcfg}-%{cfg.system}-%{cfg.architecture}"

VULKAN_SDK = "C:/VulkanSDK/1.4.304.1"

-- Organize include directories
IncludeDir = {}
IncludeDir["GLFW"] = "EvaEngine/vendor/GLFW/include"
IncludeDir["GLAD"] = "EvaEngine/vendor/GLAD/include"
IncludeDir["ImGui"] = "EvaEngine/vendor/imgui"
IncludeDir["glm"] = "EvaEngine/vendor/glm"
IncludeDir["entt"] = "EvaEngine/vendor/entt/include"
IncludeDir["stb_image"] = "EvaEngine/vendor/stb_image"
IncludeDir["yaml_cpp"] = "EvaEngine/vendor/yaml-cpp/include"
IncludeDir["ImGuizmo"] = "EvaEngine/vendor/ImGuizmo"
IncludeDir["VulkanSDK"] = "%{VULKAN_SDK}/Include"
IncludeDir["shaderc"] = "%{VULKAN_SDK}/Include/shaderc"
IncludeDir["SPIRV_Cross"] = "%{VULKAN_SDK}/Include/spirv_cross"

-- Organize library directories
LibraryDir = {}
LibraryDir["VulkanSDK"] = "%{VULKAN_SDK}/Lib"

-- Organize libraries with separate Debug and Release versions
Library = {}
-- Common libraries
Library["GLFW"] = "GLFW"
Library["GLAD"] = "GLAD"
Library["ImGui"] = "imgui"
Library["yaml_cpp"] = "yaml-cpp"
Library["OpenGL"] = "opengl32.lib"
Library["Vulkan"] = "vulkan-1.lib"

-- Debug specific libraries
Library["shaderc_Debug"] = "shaderc_sharedd.lib"
Library["spirv_cross_core_Debug"] = "spirv-cross-cored.lib"
Library["spirv_cross_glsl_Debug"] = "spirv-cross-glsld.lib"
Library["spirv_tools_Debug"] = "SPIRV-Toolsd.lib"

-- Release specific libraries
Library["shaderc_Release"] = "shaderc_shared.lib"
Library["spirv_cross_core_Release"] = "spirv-cross-core.lib"
Library["spirv_cross_glsl_Release"] = "spirv-cross-glsl.lib"
Library["spirv_tools_Release"] = "SPIRV-Tools.lib"

include "EvaEngine/vendor/GLFW"
include "EvaEngine/vendor/GLAD"
include "EvaEngine/vendor/imgui"
include "EvaEngine/vendor/yaml-cpp"

-- Define the "EvaEngine" project
project "EvaEngine"
    location "EvaEngine"  -- The directory where project files are generated
    kind "StaticLib"      -- The project outputs a static library 
    language "C++"        -- The programming language used
    cppdialect "C++17"    -- Use C++17 standard
    staticruntime "off"   -- Use dynamic runtime

    -- Directories for the output of compiled binaries and intermediate object files
    targetdir ("bin/" .. outputdir .. "/%{prj.name}")
    objdir ("bin-obj/" .. outputdir .. "/%{prj.name}")

    pchheader "pch.h"
    pchsource "EvaEngine/source/pch.cpp"

    -- Specify the files to include in the project compilation
    files
    {
        "%{prj.name}/source/pch.cpp",  -- Add the pch.cpp here explicitly
        "%{prj.name}/source/**.h",  
        "%{prj.name}/source/**.cpp", 
        "%{prj.name}/vendor/glm/glm/**.hpp",
        "%{prj.name}/vendor/glm/glm/**.inl",
        "%{prj.name}/vendor/stb_image/**.cpp",
        "%{prj.name}/vendor/stb_image/**.h",
        "%{prj.name}/vendor/ImGuizmo/ImGuizmo.cpp",
        "%{prj.name}/vendor/ImGuizmo/ImGuizmo.h",
    }

    defines
    {
        "_CRT_SECURE_NO_WARNINGS",
        "GLFW_INCLUDE_NONE",
        "YAML_CPP_STATIC_DEFINE"
    }

    -- Include directories required for the project
    includedirs
    {
        "EvaEngine/source",
        "EvaEngine/vendor/spdlog/include",  -- Include path for the spdlog logging library
        "%{IncludeDir.GLFW}",
        "%{IncludeDir.GLAD}",
        "%{IncludeDir.ImGui}",
        "%{IncludeDir.glm}",
        "%{IncludeDir.stb_image}",
        "%{IncludeDir.entt}",
        "%{IncludeDir.yaml_cpp}",
        "%{IncludeDir.ImGuizmo}",
        "%{IncludeDir.VulkanSDK}",
        "%{IncludeDir.shaderc}",
        "%{IncludeDir.SPIRV_Cross}"
    }

    libdirs
    {
        "%{LibraryDir.VulkanSDK}"
    }

    -- Apply settings specifically when building for Windows
    filter "system:windows"
        systemversion "latest"  -- Use the latest Windows SDK

        -- Preprocessor definitions for the Windows platform
        defines
        {
            "EE_PLATFORM_WINDOWS",  -- Indicates Windows platform
            "EE_BUILD_DLL",         -- Indicates the build is for a DLL
            "GLFW_INCLUDE_NONE",
            "SPDLOG_NO_UNICODE"
        } 

    -- Settings specific to the Debug configuration
    filter "configurations:Debug"
        defines "EE_DEBUG"  -- Define a macro for debug configuration
        symbols "On"        -- Enable debug symbols
        runtime "Debug"
        
        links
        {
            "%{Library.GLFW}",
            "%{Library.OpenGL}",
            "%{Library.GLAD}",
            "%{Library.ImGui}",
            "%{Library.yaml_cpp}",
            "%{Library.shaderc_Debug}",
            "%{Library.spirv_cross_core_Debug}",
            "%{Library.spirv_cross_glsl_Debug}",
            "%{Library.spirv_tools_Debug}",
            "%{Library.Vulkan}"
        }

    -- Settings specific to the Release configuration
    filter "configurations:Release"
        defines "EE_RELEASE"  -- Define a macro for release configuration
        optimize "On"         -- Enable code optimization
        runtime "Release"
        
        links
        {
            "%{Library.GLFW}",
            "%{Library.OpenGL}",
            "%{Library.GLAD}",
            "%{Library.ImGui}",
            "%{Library.yaml_cpp}",
            "%{Library.shaderc_Release}",
            "%{Library.spirv_cross_core_Release}",
            "%{Library.spirv_cross_glsl_Release}",
            "%{Library.spirv_tools_Release}",
            "%{Library.Vulkan}"
        }

    -- Settings specific to the Distribution configuration
    filter "configurations:Dist"
        defines "EE_DIST"     -- Define a macro for distribution configuration
        optimize "On"         -- Enable code optimization
        runtime "Release"
        
        links
        {
            "%{Library.GLFW}",
            "%{Library.OpenGL}",
            "%{Library.GLAD}",
            "%{Library.ImGui}",
            "%{Library.yaml_cpp}",
            "%{Library.shaderc_Release}",
            "%{Library.spirv_cross_core_Release}",
            "%{Library.spirv_cross_glsl_Release}",
            "%{Library.spirv_tools_Release}",
            "%{Library.Vulkan}"
        }

-- Shared function to set up application projects with common settings
function SetupAppProject(projectName)
    project(projectName)
        location(projectName)
        kind "ConsoleApp"
        language "C++"
        architecture "x64"
        staticruntime "off"
        cppdialect "C++17"

        targetdir ("bin/" .. outputdir .. "/%{prj.name}")
        objdir ("bin-obj/" .. outputdir .. "/%{prj.name}")

        files
        {
            "%{prj.name}/source/**.h",
            "%{prj.name}/source/**.cpp"
        }

        includedirs
        {
            "EvaEngine/vendor/spdlog/include",
            "EvaEngine/source",
            "EvaEngine/vendor",
            "%{IncludeDir.glm}",
            "%{IncludeDir.entt}",
        }

        links
        {
            "EvaEngine"
        }

        filter "system:windows"
            systemversion "latest"
            defines
            {
                "EE_PLATFORM_WINDOWS"
            }

        filter "configurations:Debug"
            defines "EE_DEBUG"
            symbols "On"
            runtime "Debug"

        filter "configurations:Release"
            defines "EE_RELEASE"
            optimize "On"
            runtime "Release"

        filter "configurations:Dist"
            defines "EE_DIST"
            optimize "On"
            runtime "Release"
end

-- Setup the Sandbox application project
SetupAppProject("Sandbox")

-- Setup the Editor application project
SetupAppProject("Editor")