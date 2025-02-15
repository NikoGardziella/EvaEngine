project "GLAD"
	kind "StaticLib"
	language "C"
	staticruntime "On" --staticly linking the runtime libraries

	targetdir ("bin/" .. outputdir .. "/%{prj.name}")
	objdir ("bin-int/" .. outputdir .. "/%{prj.name}")


	files
	{
		"include/glad/glad.h",
		"include/KHR/khrplatform.h",
		"src/glad.c"
	}

	includedirs
	{
		"include"
	}


	filter "system:windows"
		systemversion "latest"


    -- Settings specific to the Debug configuration
    filter "configurations:Debug"
        symbols "On"        -- Enable debug symbols
        runtime "Debug"

    -- Settings specific to the Release configuration
    filter "configurations:Release"
        optimize "On"         -- Enable code optimization
        runtime "Release"
