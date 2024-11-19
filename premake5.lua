-- Define the workspace, its name, and the architecture used
workspace "EvaEngine"
    architecture "x64"  -- Specify 64-bit architecture

    -- Define the available build configurations
    configurations
    {
        "Debug",     -- Configuration for development with debug symbols
        "Release",   -- Configuration for optimized build without debug symbols
        "Dist"       -- Configuration for distribution (final product) with high optimization
    }

-- Variable to determine the output directory format based on configuration, system, and architecture
outputdir = "%{cfg.buildcfg}-%{cfg.system}-%{cfg.architecture}"

-- Define the "EvaEngine" project
project "EvaEngine"
    location "EvaEngine"  -- The directory where project files are generated
    kind "SharedLib"      -- The project outputs a shared library (DLL)
    language "C++"        -- The programming language used
  
    -- Directories for the output of compiled binaries and intermediate object files
    targetdir ("bin/" .. outputdir .. "/%{prj.name}")
    objdir ("bin-obj/" .. outputdir .. "/%{prj.name}")

    -- Specify the files to include in the project compilation
    files
    {
        "%{prj.name}/source/**.h",  -- All header files in the source directory
        "%{prj.name}/source/**.cpp" -- All C++ source files in the source directory
    }

    -- Include directories required for the project
    includedirs
    {
        "EvaEngine/vendor/spdlog/include"  -- Include path for the spdlog logging library
    }

    -- Apply settings specifically when building for Windows
    filter "system:windows"
        cppdialect "C++17"          -- Use C++17 standard
        staticruntime "On"          -- Link the runtime library statically
        systemversion "latest"      -- Use the latest Windows SDK

        -- Preprocessor definitions for the Windows platform
        defines
        {
            "EE_PLATFORM_WINDOWS",  -- Indicates Windows platform
            "EE_BUILD_DLL"          -- Indicates the build is for a DLL
        }

        -- Post-build commands to copy the DLL to the Sandbox project directory
        postbuildcommands
        { 
            -- Create the directory if it doesn't exist
            'if not exist "$(SolutionDir)bin\\' .. outputdir .. '\\Sandbox" mkdir "$(SolutionDir)bin\\' .. outputdir .. '\\Sandbox"',
            -- Copy the compiled DLL to the Sandbox directory
            'xcopy /Y /Q "$(SolutionDir)bin\\' .. outputdir .. '\\EvaEngine\\EvaEngine.dll" "$(SolutionDir)bin\\' .. outputdir .. '\\Sandbox\\"'
        }

    -- Settings specific to the Debug configuration
    filter "configurations:Debug"
        defines "EE_DEBUG"  -- Define a macro for debug configuration
        symbols "On"        -- Enable debug symbols
        buildoptions "/utf-8"  -- Use UTF-8 character encoding for source files

    -- Settings specific to the Release configuration
    filter "configurations:Release"
        defines "EE_RELEASE"  -- Define a macro for release configuration
        optimize "On"         -- Enable code optimization
        buildoptions "/utf-8" -- Use UTF-8 character encoding

    -- Settings specific to the Distribution configuration
    filter "configurations:Dist"
        defines "EE_DIST"     -- Define a macro for distribution configuration
        optimize "On"         -- Enable code optimization
        buildoptions "/utf-8" -- Use UTF-8 character encoding

-- Define the "Sandbox" project
project "Sandbox"
    location "Sandbox"     -- The directory where project files are generated
    kind "ConsoleApp"      -- The project outputs a console application
    language "C++"         -- The programming language used
    architecture "x64"     -- Specify 64-bit architecture

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
        "EvaEngine/source"                  -- Include path for EvaEngine source
    }

    -- Link the Sandbox project to the EvaEngine library
    links
    {
        "EvaEngine"
    }

    -- Apply settings specifically when building for Windows
    filter "system:windows"
        cppdialect "C++17"          -- Use C++17 standard
        staticruntime "On"          -- Link the runtime library statically
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
        buildoptions "/utf-8"  -- Use UTF-8 character encoding for source files

    -- Settings specific to the Release configuration
    filter "configurations:Release"
        defines "EE_RELEASE"  -- Define a macro for release configuration
        optimize "On"         -- Enable code optimization
        buildoptions "/utf-8" -- Use UTF-8 character encoding

    -- Settings specific to the Distribution configuration
    filter "configurations:Dist"
        defines "EE_DIST"     -- Define a macro for distribution configuration
        optimize "On"         -- Enable code optimization
        buildoptions "/utf-8" -- Use UTF-8 character encoding