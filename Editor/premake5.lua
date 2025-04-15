--VULKAN_SDK = "C:/VulkanSDK/1.4.304.1"
--IncludeDir = {}
 --IncludeDir["VulkanSDK"] = "%{VULKAN_SDK}/Include"

-- Setup the Editor application project
project "Editor"
    location "."
    kind "ConsoleApp"
    language "C++"
    architecture "x64"
    staticruntime "off"
    cppdialect "C++17"

    targetdir ("bin/" .. outputdir .. "/%{prj.name}")
    objdir ("bin-obj/" .. outputdir .. "/%{prj.name}")


    files
    {
        "source/**.h",
        "source/**.cpp",
        "../Sandbox/source/Sandbox2D.cpp",
        "../Sandbox/source/SandboxApp.cpp",
        "../Sandbox/source/Sandbox2D.h"
    }



    includedirs
    {
        "../EvaEngine/vendor/spdlog/include",
        "../EvaEngine/source",
        "../EvaEngine/vendor",
        "../EvaEngine/vendor/glm",
        "../EvaEngine/vendor/entt/include",
        "../EvaEngine/vendor/Box2D/include",
        "../EvaEngine/vendor/enkiTS/src",
        "../EvaEngine/vendor/GLFW/include",
        "../Sandbox/source", 
        "../Sandbox/vendor",
        "%{IncludeDir.ImGui}",

        "%{IncludeDir.VulkanSDK}",

    }


    links
    {
        "EvaEngine",
        "Sandbox"
        
    }

    filter "system:windows"
        systemversion "latest"
        defines
        {
            "EE_EDITOR",
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