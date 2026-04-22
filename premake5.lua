-- premake5.lua (repo root)

include "Dependencies.lua"

-- MUST be defined before any included scripts that use it
outputdir = "%{cfg.buildcfg}-%{cfg.system}-%{cfg.architecture}"

-- Common path constants
ENGINE_DIR = "EvaEngine"
VENDOR_DIR = ENGINE_DIR .. "/vendor"

local ACTION = _ACTION or "vs2022"

workspace "EvaEngine"
    architecture "x86_64"
    startproject "Editor"
    configurations { "Debug", "Release", "Dist" }

    -- Keep generated solution/projects out of repo root
    location ("build/" .. ACTION)

    -- Apply globally on Windows (spdlog/fmt wants this)
    filter "system:windows"
        systemversion "latest"
        buildoptions { "/utf-8" }
    filter {}

group "Dependencies"
    include "premake/box2d.lua"
    include "premake/glad.lua"
    include "premake/glfw.lua"
    include "premake/imgui.lua"
    include "premake/nlohmannjson.lua"
    include "premake/yaml-cpp.lua"
group ""

group "Apps"
    include "Game"
    include "Editor"
group ""


-- Common path constants (keeps wrappers and projects consistent)
ENGINE_DIR = "EvaEngine"
VENDOR_DIR = ENGINE_DIR .. "/vendor"

outputdir = "%{cfg.buildcfg}-%{cfg.system}-%{cfg.architecture}"

-- 3rd-party wrappers (tracked in your repo)

-- Your projects
include "Game"
include "Editor"

project "EvaEngine"
    location (ENGINE_DIR)
    kind "StaticLib"
    language "C++"
    cppdialect "C++17"
    staticruntime "off"

filter "system:windows"
    -- Use the VENDOR_DIR constant you already defined!
    includedirs { VENDOR_DIR .. "/vcpkg_installed/x64-windows/include" }
    libdirs     { VENDOR_DIR .. "/vcpkg_installed/x64-windows/lib" }
    links       { "libcurl", "zlib" }
filter {}

    filter "files:**/vendor/ImGuizmo/ImGuizmo.cpp or files:**/vendor/enkiTS/src/TaskScheduler.cpp"
        pchsource ""
    filter {}
    filter "files:**/vendor/ImGuizmo/ImGuizmo.cpp"
        buildoptions { "/Y-" }
    filter "files:**/vendor/enkiTS/src/TaskScheduler.cpp"
        buildoptions { "/Y-" }
    filter {}

    filter { "files:**/vendor/lz4/lz4.c" }
        enablepch "Off"
    filter {}

    multiprocessorcompile "On"

    targetdir ("bin/" .. outputdir .. "/%{prj.name}")
    objdir    ("bin-obj/" .. outputdir .. "/%{prj.name}")

    pchheader "pch.h"
    pchsource (ENGINE_DIR .. "/source/pch.cpp")

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

        -- NOTE: double-check these two paths are correct in your repo
        "%{prj.name}/vendor/Box2D/include/**.h",
        "%{prj.name}/vendor/enkiTS/src/TaskScheduler.h",
        "%{prj.name}/vendor/enkiTS/src/TaskScheduler.cpp",
        "%{prj.name}/vendor/lz4/lz4.h",
        "%{prj.name}/vendor/lz4/lz4_wrapper.cpp",
        "%{prj.name}/vendor/lz4/lz4hc.h",
        "%{prj.name}/vendor/lz4/lz4hc_wrapper.cpp",
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
        ENGINE_DIR .. "/source",
        VENDOR_DIR .. "/spdlog/include",
        
        "Editor/source",

        "%{IncludeDir.vcpkg}",

        -- From Dependencies.lua
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
        "%{IncludeDir.lz4}"
    }

    libdirs
    {
        "%{LibraryDir.VulkanSDK}",
        "%{LibraryDir.vcpkg}"
    }

    filter "system:windows"
        systemversion "latest"
        defines
        {
            "EE_PLATFORM_WINDOWS",
            "EE_BUILD_DLL",
            "SPDLOG_NO_UNICODE"
        }

    filter "configurations:Debug"
        defines
        {
            "EE_DEBUG",
            "EE_PROFILE=0"
        }
        symbols "On"
        runtime "Debug"
        buildoptions { "/Z7" }
        linkoptions  { "/DEBUG:FASTLINK" }
        optimize "Off"
        links
        {
            "GLFW",
            "GLAD",
            "ImGui",
            "yaml-cpp",
            "Box2D",

            "%{Library.OpenGL}",
            "%{Library.shaderc_Debug}",
            "%{Library.spirv_cross_core_Debug}",
            "%{Library.spirv_cross_glsl_Debug}",
            "%{Library.spirv_tools_Debug}",
            "%{Library.Vulkan}",

            "%{Library.libcurl}",
            "%{Library.zlib}"
        }



    filter "configurations:Release"
        defines
        {
            "EE_RELEASE",
            "EE_PROFILE=1"
        }
        optimize "Speed"
        runtime "Release"
        linktimeoptimization "On"
        links
        {
            "GLFW",
            "GLAD",
            "ImGui",
            "yaml-cpp",
            "Box2D",

            "%{Library.OpenGL}",
            "%{Library.shaderc_Release}",
            "%{Library.spirv_cross_core_Release}",
            "%{Library.spirv_cross_glsl_Release}",
            "%{Library.spirv_tools_Release}",
            "%{Library.Vulkan}",

            "%{Library.libcurl}",
            "%{Library.zlib}"
        }



    filter "configurations:Dist"
        defines
        {
            "EE_DIST",
            "EE_PROFILE=0"
        }
        optimize "Speed"
        runtime "Release"
        linktimeoptimization "On"
        links
        {
            "GLFW",
            "GLAD",
            "ImGui",
            "yaml-cpp",
            "Box2D",

            "%{Library.OpenGL}",
            "%{Library.shaderc_Release}",
            "%{Library.spirv_cross_core_Release}",
            "%{Library.spirv_cross_glsl_Release}",
            "%{Library.spirv_tools_Release}",
            "%{Library.Vulkan}",

            "%{Library.libcurl}",
            "%{Library.zlib}"
        }


    filter {}
