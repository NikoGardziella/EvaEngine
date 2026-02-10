project "GLAD"
    kind "StaticLib"
    language "C"
    staticruntime "off"

    targetdir ("bin/" .. outputdir .. "/%{prj.name}")
    objdir    ("bin-int/" .. outputdir .. "/%{prj.name}")

   local ROOT = _MAIN_SCRIPT_DIR
local glad = path.join(ROOT, "EvaEngine/vendor/GLAD")

    files
    {
        glad .. "/include/glad/glad.h",
        glad .. "/include/KHR/khrplatform.h",
        glad .. "/src/glad.c"
    }

    includedirs
    {
        glad .. "/include"
    }

    filter "system:windows"
        systemversion "latest"

    filter "configurations:Debug"
        symbols "On"
        runtime "Debug"

    filter "configurations:Release or Dist"
        optimize "On"
        runtime "Release"

    filter {}
