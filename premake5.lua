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

IncludeDir = {}
IncludeDir["GLFW"] = "EvaEngine/vendor/GLFW/include"
IncludeDir["GLAD"] = "EvaEngine/vendor/GLAD/include"
IncludeDir["ImGui"] = "EvaEngine/vendor/imgui"
IncludeDir["glm"] = "EvaEngine/vendor/glm"
IncludeDir["entt"] = "EvaEngine/vendor/entt/include"
IncludeDir["stb_image"] = "EvaEngine/vendor/stb_image"

include "EvaEngine/vendor/GLFW"
include "EvaEngine/vendor/GLAD"
include "EvaEngine/vendor/imgui"

-- Define the "EvaEngine" project
project "EvaEngine"
    location "EvaEngine"  -- The directory where project files are generated
    kind "StaticLib"      -- The project outputs a static library 
    language "C++"        -- The programming language used
    cppdialect "C++17"          -- Use C++17 standard
    staticruntime "on"

    -- Directories for the output of compiled binaries and intermediate object files
    targetdir ("bin/" .. outputdir .. "/%{prj.name}")
    objdir ("bin-obj/" .. outputdir .. "/%{prj.name}")

    pchheader "pch.h"
    pchsource "EvaEngine/source/pch.cpp"

    -- Specify the files to include in the project compilation
    files
    {
        "%{prj.name}/source/**.h",  -- All header files in the source directory
        "%{prj.name}/source/**.cpp", -- All C++ source files in the source directory
        "%{prj.name}/vendor/glm/glm/**.hpp",
        "%{prj.name}/vendor/glm/glm/**.inl",
        "%{prj.name}/vendor/stb_image/**.cpp",
        "%{prj.name}/vendor/stb_image/**.h",
    }

    defines
    {
        "_CRT_SECURE_NO_WARNINGS",
		"GLFW_INCLUDE_NONE"
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
        "%{IncludeDir.entt}"
    }
    links
    {
        "GLFW",
        "opengl32.lib",
        "GLAD",
        "imgui"
    }

    -- Apply settings specifically when building for Windows
    filter "system:windows"
        systemversion "latest"      -- Use the latest Windows SDK

        -- Preprocessor definitions for the Windows platform
        defines
        {
            "EE_PLATFORM_WINDOWS",  -- Indicates Windows platform
            "EE_BUILD_DLL",          -- Indicates the build is for a DLL
            "GLFW_INCLUDE_NONE",
            "SPDLOG_NO_UNICODE"

        }

       

    -- Settings specific to the Debug configuration
    filter "configurations:Debug"
        defines "EE_DEBUG"  -- Define a macro for debug configuration
        symbols "On"        -- Enable debug symbols
        runtime "Debug"
        --buildoptions "/utf-8"-- Use UTF-8 character encoding for source files

    -- Settings specific to the Release configuration
    filter "configurations:Release"
        defines "EE_RELEASE"  -- Define a macro for release configuration
        optimize "On"         -- Enable code optimization
        runtime "Release"
        --buildoptions "/utf-8" -- Use UTF-8 character encoding

    -- Settings specific to the Distribution configuration
    filter "configurations:Dist"
        defines "EE_DIST"     -- Define a macro for distribution configuration
        optimize "On"         -- Enable code optimization
        runtime "Release"
        --buildoptions "/utf-8"  -- Use UTF-8 character encoding

-- Define the "Sandbox" project
project "Sandbox"
    location "Sandbox"     -- The directory where project files are generated
    kind "ConsoleApp"      -- The project outputs a console application
    language "C++"         -- The programming language used
    architecture "x64"     -- Specify 64-bit architecture
    staticruntime "on"          -- Link the runtime library statically
    cppdialect "C++17"          -- Use C++17 standard

    -- Directories for the output of compiled binaries and intermediate object files
    targetdir ("bin/" .. outputdir .. "/%{prj.name}")
    objdir ("bin-obj/" .. outputdir .. "/%{prj.name}")

    -- Specify the files to include in the project compilation
    files
    {
        "%{prj.name}/source/**.h",  -- All header files in the source directory
        "%{prj.name}/source/**.cpp" -- All C++ source files in the source directory
    }

    -- Include directories required for the Sandbox project
    includedirs
    {
        "EvaEngine/vendor/spdlog/include",  -- Include path for the spdlog library
        "EvaEngine/source",                 -- Include path for EvaEngine source
        "EvaEngine/vendor",
        "%{IncludeDir.glm}",
        "%{IncludeDir.entt}",
    }

    -- Link the Sandbox project to the EvaEngine library
    links
    {
        "EvaEngine"
    }

    -- Apply settings specifically when building for Windows
    filter "system:windows"
        cppdialect "C++17"          -- Use C++17 standard
        systemversion "latest"      -- Use the latest Windows SDK

        -- Preprocessor definitions for the Windows platform
        defines
        {
            "EE_PLATFORM_WINDOWS"  -- Indicates Windows platform
        }

    -- Settings specific to the Debug configuration
    filter "configurations:Debug"
        defines "EE_DEBUG"  -- Define a macro for debug configuration
        symbols "On"        -- Enable debug symbols
        runtime "Debug"
        --buildoptions "/utf-8"  -- Use UTF-8 character encoding

    -- Settings specific to the Release configuration
    filter "configurations:Release"
        defines "EE_RELEASE"  -- Define a macro for release configuration
        optimize "On"         -- Enable code optimization
        runtime "Release"
        --buildoptions "/utf-8"  -- Use UTF-8 character encoding

    -- Settings specific to the Distribution configuration
    filter "configurations:Dist"
        defines "EE_DIST"     -- Define a macro for distribution configuration
        optimize "On"         -- Enable code optimization
        runtime "Release"
        --buildoptions "/utf-8"  -- Use UTF-8 character encoding


project "Editor"
    location "Editor"     -- The directory where project files are generated
    kind "ConsoleApp"      -- The project outputs a console application
    language "C++"         -- The programming language used
    architecture "x64"     -- Specify 64-bit architecture
    staticruntime "on"          -- Link the runtime library statically
    cppdialect "C++17"          -- Use C++17 standard

    -- Directories for the output of compiled binaries and intermediate object files
    targetdir ("bin/" .. outputdir .. "/%{prj.name}")
    objdir ("bin-obj/" .. outputdir .. "/%{prj.name}")

    -- Specify the files to include in the project compilation
    files
    {
        "%{prj.name}/source/**.h",  -- All header files in the source directory
        "%{prj.name}/source/**.cpp" -- All C++ source files in the source directory
    }

    -- Include directories required for the Sandbox project
    includedirs
    {
        "EvaEngine/vendor/spdlog/include",  -- Include path for the spdlog library
        "EvaEngine/source",                 -- Include path for EvaEngine source
        "EvaEngine/vendor",
        "%{IncludeDir.glm}",
        "%{IncludeDir.entt}",
    }

    -- Link the Sandbox project to the EvaEngine library
    links
    {
        "EvaEngine"
    }

    -- Apply settings specifically when building for Windows
    filter "system:windows"
        cppdialect "C++17"          -- Use C++17 standard
        systemversion "latest"      -- Use the latest Windows SDK

        -- Preprocessor definitions for the Windows platform
        defines
        {
            "EE_PLATFORM_WINDOWS"  -- Indicates Windows platform
        }

    -- Settings specific to the Debug configuration
    filter "configurations:Debug"
        defines "EE_DEBUG"  -- Define a macro for debug configuration
        symbols "On"        -- Enable debug symbols
        runtime "Debug"
        --buildoptions "/utf-8"  -- Use UTF-8 character encoding

    -- Settings specific to the Release configuration
    filter "configurations:Release"
        defines "EE_RELEASE"  -- Define a macro for release configuration
        optimize "On"         -- Enable code optimization
        runtime "Release"
        --buildoptions "/utf-8"  -- Use UTF-8 character encoding

    -- Settings specific to the Distribution configuration
    filter "configurations:Dist"
        defines "EE_DIST"     -- Define a macro for distribution configuration
        optimize "On"         -- Enable code optimization
        runtime "Release"
        --buildoptions "/utf-8"  -- Use UTF-8 character encoding
