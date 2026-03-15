project "Editor"
    -- location "Editor" -- optional
    kind "ConsoleApp"
    language "C++"
    architecture "x64"
    staticruntime "off"
    cppdialect "C++17"

    targetdir ("build/bin/" .. outputdir .. "/%{prj.name}")
    objdir    ("build/bin-int/" .. outputdir .. "/%{prj.name}")
    targetname "Editor"

	postbuildcommands 
	{
 	   "{COPY} \"../../EvaEngine/vendor/vcpkg_installed/x64-windows/bin/*.dll\" \"%{cfg.targetdir}\""
	}

    files
    {
        "source/**.h",
        "source/**.cpp"
    }

    includedirs
    {
        "../EvaEngine/source",
        "../EvaEngine/vendor/spdlog/include",
        "%{IncludeDir.ImGuizmo}",
        "%{IncludeDir.glm}",
        "%{IncludeDir.entt}",
        "%{IncludeDir.enkiTS}",
        "%{IncludeDir.Box2D}",
        "%{IncludeDir.GLFW}",
        "%{IncludeDir.ImGui}",
        "%{IncludeDir.yaml_cpp}",
        "%{IncludeDir.VulkanSDK}",
        "%{IncludeDir.vcpkg}",
        "%{IncludeDir.stb_image}",
    }

    links
    {
        "EvaEngine",
        "Game",
        "yaml-cpp",
        "Box2D"
    }

    filter "system:windows"
        systemversion "latest"
        buildoptions { "/utf-8" }
        defines
        {
            "EE_PLATFORM_WINDOWS",
            "YAML_CPP_STATIC_DEFINE"
        }

    filter "configurations:Debug"
        defines { "EE_DEBUG" }
        symbols "On"
        runtime "Debug"

    filter "configurations:Release"
        defines { "EE_RELEASE" }
        optimize "On"
        runtime "Release"

    filter "configurations:Dist"
        defines { "EE_DIST" }
        optimize "On"
        runtime "Release"

    filter {}

