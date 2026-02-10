project "ImGui"
    kind "StaticLib"
    language "C++"
    cppdialect "C++17"
    staticruntime "off"

    targetdir ("bin/" .. outputdir .. "/%{prj.name}")
    objdir    ("bin-int/" .. outputdir .. "/%{prj.name}")

    local ROOT = _MAIN_SCRIPT_DIR
    local imgui = path.join(ROOT, "EvaEngine/vendor/imgui")

    files
    {
        imgui .. "/imconfig.h",
        imgui .. "/imgui.h",
        imgui .. "/imgui.cpp",
        imgui .. "/imgui_draw.cpp",
        imgui .. "/imgui_internal.h",
        imgui .. "/imgui_widgets.cpp",
        imgui .. "/imstb_rectpack.h",
        imgui .. "/imstb_textedit.h",
        imgui .. "/imstb_truetype.h",
        imgui .. "/imgui_demo.cpp",
        imgui .. "/imgui_tables.cpp"

        -- If you use backends, add these (recommended for Vulkan+GLFW):
        --, imgui .. "/backends/imgui_impl_glfw.h"
        --, imgui .. "/backends/imgui_impl_glfw.cpp"
        --, imgui .. "/backends/imgui_impl_vulkan.h"
        --, imgui .. "/backends/imgui_impl_vulkan.cpp"
    }

    includedirs
    {
        imgui,
        imgui .. "/backends"
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
