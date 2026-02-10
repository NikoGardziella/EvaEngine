project "Game"
    -- location "Game"  -- optional; you can set this if you want the vcxproj in /Game
    kind "StaticLib"
    language "C++"
    cppdialect "C++17"
    staticruntime "off"
    architecture "x64"

    targetdir ("build/bin/" .. outputdir .. "/%{prj.name}")
    objdir    ("build/bin-int/" .. outputdir .. "/%{prj.name}")

    files
    {
        "source/**.h",
        "source/**.cpp"
    }

    includedirs
    {
        "../EvaEngine/source",
        "../EvaEngine/vendor/spdlog/include",

        "%{IncludeDir.glm}",
        "%{IncludeDir.entt}",
        "%{IncludeDir.enkiTS}",
        "%{IncludeDir.Box2D}",
        "%{IncludeDir.GLFW}",
        "%{IncludeDir.VulkanSDK}"
    }

    links
    {
        "EvaEngine",
        "Box2D"
    }

    filter "system:windows"
        systemversion "latest"
        buildoptions { "/utf-8" }
        defines
        {
            "EE_SANDBOX",
            "EE_PLATFORM_WINDOWS"
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
