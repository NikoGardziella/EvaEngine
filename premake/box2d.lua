project "Box2D"
    kind "StaticLib"
    language "C++"
    cppdialect "C++17"
    staticruntime "off"

    local root  = path.getabsolute(_MAIN_SCRIPT_DIR)
    local box2d = path.join(root, "EvaEngine/vendor/Box2D")

    targetdir (path.join(root, "bin", outputdir, "%{prj.name}"))
    objdir    (path.join(root, "bin-obj", outputdir, "%{prj.name}"))

    files
    {
        box2d .. "/src/**.c",
        box2d .. "/src/**.h",
        box2d .. "/include/**.h"
    }

    includedirs
    {
        box2d .. "/include",
        box2d .. "/src"
    }

    filter "system:windows"
        systemversion "latest"
        buildoptions { "/std:c17" }

    filter "configurations:Debug"
        runtime "Debug"
        symbols "On"

    filter "configurations:Release or Dist"
        runtime "Release"
        optimize "On"

    filter {}
