include "Dependencies.lua"

workspace "EvaEngine"
    architecture "x86_64"
    startproject "Editor"

    configurations
    {
        "Debug",
        "Release",
        "Dist"
    }

outputdir = "%{cfg.buildcfg}-%{cfg.system}-%{cfg.architecture}"

include "EvaEngine/vendor/GLFW"
include "EvaEngine/vendor/GLAD"
include "EvaEngine/vendor/imgui"
include "EvaEngine/vendor/yaml-cpp"
include "EvaEngine/vendor/Box2D"
include "Game"
include "Editor"

project "EvaEngine"
    location "EvaEngine"
    kind "StaticLib"
    language "C++"
    cppdialect "C++17"
    staticruntime "off"
    
    flags { "MultiProcessorCompile" }
    buildoptions { "/MP" } 

    targetdir ("bin/" .. outputdir .. "/%{prj.name}")
    objdir ("bin-obj/" .. outputdir .. "/%{prj.name}")

    pchheader "pch.h"
    pchsource "EvaEngine/source/pch.cpp"

    files
    {
        "%{prj.name}/source/pch.cpp",
        "%{prj.name}/source/**.h",  
        "%{prj.name}/source/**.cpp", 
        "%{prj.name}/vendor/glm/glm/**.hpp",
        "%{prj.name}/vendor/glm/glm/**.inl",
        "%{prj.name}/vendor/stb_image/**.cpp",
        "%{prj.name}/vendor/stb_image/**.h",
        "%{prj.name}/vendor/tiny_gltf/**.h",
        "%{prj.name}/vendor/tiny_gltf/**.cpp",
        "%{prj.name}/vendor/ImGuizmo/ImGuizmo.cpp",
        "%{prj.name}/vendor/ImGuizmo/ImGuizmo.h",
        "%{prj.name}/vendor/Box2D/box2d/include/**.h",
        "%{prj.name}/vendor/enkiTS/box2d/src/TaskScheduler.h",
        "%{prj.name}/vendor/enkiTS/src/TaskScheduler.cpp", 
        "assets/shaders/*"
    }

    defines
    {
        "_CRT_SECURE_NO_WARNINGS",
        "GLFW_INCLUDE_NONE",
        "YAML_CPP_STATIC_DEFINE"
    }

    includedirs
    {
        "EvaEngine/source",
        "EvaEngine/vendor/spdlog/include",
        "EvaEngine/vendor/vcpkg/x64-windows/include",
        "%{IncludeDir.GLFW}",
        "%{IncludeDir.GLAD}",
        "%{IncludeDir.json}",
        "%{IncludeDir.Box2D}",
        "%{IncludeDir.ImGui}",
        "%{IncludeDir.glm}",
        "%{IncludeDir.stb_image}",
        "%{IncludeDir.entt}",
        "%{IncludeDir.enkiTS}",
        "%{IncludeDir.yaml_cpp}",
        "%{IncludeDir.ImGuizmo}",
        "%{IncludeDir.VulkanSDK}",
        "%{IncludeDir.shaderc}",
        "%{IncludeDir.SPIRV_Cross}",
        "%{IncludeDir.tiny_gltf}",
    }

    libdirs
    {
        "%{LibraryDir.VulkanSDK}",
        "%{LibraryDir.Box2D}",
        "EvaEngine/vendor/vcpkg/x64-windows/lib"
    }

    filter "system:windows"
        systemversion "latest"

        defines
        {
            "EE_PLATFORM_WINDOWS",
            "EE_BUILD_DLL",
            "GLFW_INCLUDE_NONE",
            "SPDLOG_NO_UNICODE"
        }


    filter "configurations:Debug"
        defines { 
            "EE_DEBUG",
            "EE_PROFILE=0"  --  Disable profiling in debug for faster builds
        }
        symbols "On"
        runtime "Debug"
        
        --  Faster debug info format (works with ccache)
        buildoptions { "/Z7" }
        
        --  Faster linking
        linkoptions { "/DEBUG:FASTLINK" }
        
        --  Optional: Disable optimizations for faster iteration
        optimize "Off"
        
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
            "%{Library.Vulkan}",
            "%{Library.Box2D}",
            "libcurl"
        }

    filter "configurations:Release"
        defines { 
            "EE_RELEASE",
            "EE_PROFILE=1"  --  Enable profiling in release
        }
        optimize "Speed" 
        runtime "Release"
        
        flags { "LinkTimeOptimization" }
        
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
            "%{Library.Vulkan}",
        }

    filter "configurations:Dist"
        defines { 
            "EE_DIST",
            "EE_PROFILE=0"  --  Disable profiling in distribution
        }
        optimize "Speed"
        runtime "Release"
        
        --  Maximum optimizations for distribution
        flags { "LinkTimeOptimization" }
        
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
            "%{Library.Vulkan}",
        }