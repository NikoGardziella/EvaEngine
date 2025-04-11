
    project "Sandbox"
        location "."
        kind "ConsoleApp" 
        language "C++"
        architecture "x64"
        staticruntime "off"
        cppdialect "C++17"

        targetdir ("bin/" .. outputdir .. "/%{prj.name}")
        objdir ("bin-obj/" .. outputdir .. "/%{prj.name}")

        ROOT_DIR = path.getabsolute(os.getcwd()) 

        files
        {
            "source/**.h",
            "source/**.cpp"
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
            "%{IncludeDir.VulkanSDK}",
            "../EvaEngine/vendor/GLFW/include",


        }


        links
        {
            "EvaEngine",
            
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


