include "Dependencies.lua"

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

include "EvaEngine/vendor/GLFW"
include "EvaEngine/vendor/GLAD"
include "EvaEngine/vendor/imgui"
include "EvaEngine/vendor/yaml-cpp"
include "EvaEngine/vendor/Box2D"
include "Sandbox"
include "Editor"

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
        "%{prj.name}/vendor/Box2D/box2d/include/**.h",
        "%{prj.name}/vendor/enkiTS/box2d/src/TaskScheduler.h",
        "%{prj.name}/vendor/enkiTS/src/TaskScheduler.cpp", 
        "assets/shaders/*"  -- Match all files and subdirectories under assets
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
    }

    libdirs
    {
        "%{LibraryDir.VulkanSDK}",
        "%{LibraryDir.Box2D}"

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
            "%{Library.Vulkan}",
            "%{Library.Box2D}",
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
            "%{Library.Vulkan}",
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
            "%{Library.Vulkan}",
        }


