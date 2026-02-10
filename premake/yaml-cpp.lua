project "yaml-cpp"
    kind "StaticLib"
    language "C++"
    cppdialect "C++17"
    staticruntime "off"

    targetdir ("build/bin/" .. outputdir .. "/%{prj.name}")
    objdir    ("build/bin-int/" .. outputdir .. "/%{prj.name}")

    local ROOT = _MAIN_SCRIPT_DIR
    local yaml = path.join(ROOT, "EvaEngine/vendor/yaml-cpp")

    files
    {
        yaml .. "/src/**.h",
        yaml .. "/src/**.cpp",
        yaml .. "/include/**.h"
    }

    includedirs
    {
        yaml .. "/include"
    }

    defines
    {
        "YAML_CPP_STATIC_DEFINE"
    }

    filter "system:windows"
        systemversion "latest"

    filter "system:linux"
        systemversion "latest"
        pic "On"

    filter "configurations:Debug"
        runtime "Debug"
        symbols "On"

    filter "configurations:Release or Dist"
        runtime "Release"
        optimize "On"

    filter {}
