project "GLFW"
    kind "StaticLib"
    language "C"
    staticruntime "off"

    targetdir ("bin/" .. outputdir .. "/%{prj.name}")
    objdir    ("bin-int/" .. outputdir .. "/%{prj.name}")

    local ROOT = _MAIN_SCRIPT_DIR
    local glfw = path.join(ROOT, "EvaEngine/vendor/GLFW")

    files
    {
        glfw .. "/include/GLFW/glfw3.h",
        glfw .. "/include/GLFW/glfw3native.h",

        glfw .. "/src/platform.h",
        glfw .. "/src/mappings.h",

        glfw .. "/src/context.c",
        glfw .. "/src/init.c",
        glfw .. "/src/input.c",
        glfw .. "/src/monitor.c",
        glfw .. "/src/platform.c",
        glfw .. "/src/vulkan.c",
        glfw .. "/src/window.c",

        glfw .. "/src/egl_context.c",
        glfw .. "/src/osmesa_context.c",

        -- Null platform (safe to include; unused on Win32)
        glfw .. "/src/null_platform.h",
        glfw .. "/src/null_joystick.h",
        glfw .. "/src/null_init.c",
        glfw .. "/src/null_monitor.c",
        glfw .. "/src/null_window.c",
        glfw .. "/src/null_joystick.c"
    }

    includedirs
    {
        glfw .. "/include"
    }

    filter "system:windows"
        systemversion "latest"

        files
        {
            glfw .. "/src/win32_init.c",
            glfw .. "/src/win32_module.c",
            glfw .. "/src/win32_joystick.c",
            glfw .. "/src/win32_monitor.c",
            glfw .. "/src/win32_time.h",
            glfw .. "/src/win32_time.c",
            glfw .. "/src/win32_thread.h",
            glfw .. "/src/win32_thread.c",
            glfw .. "/src/win32_window.c",
            glfw .. "/src/wgl_context.c"
        }

        defines
        {
            "_GLFW_WIN32",
            "_CRT_SECURE_NO_WARNINGS"
        }

    filter "configurations:Debug"
        symbols "On"
        runtime "Debug"

    filter "configurations:Release or Dist"
        optimize "On"
        runtime "Release"

    filter {}
