-- Dependencies.lua

VULKAN_SDK = "C:/VulkanSDK/1.4.304.1"

ROOT_DIR   = _MAIN_SCRIPT_DIR
ENGINE_DIR = "EvaEngine"
VENDOR_DIR = ENGINE_DIR .. "/vendor"

local function R(p) return path.join(ROOT_DIR, p) end

VCPKG_TRIPLET   = "x64-windows"
VCPKG_INSTALLED = VENDOR_DIR .. "/vcpkg/installed/" .. VCPKG_TRIPLET


IncludeDir = {
    GLFW      = R(VENDOR_DIR .. "/GLFW/include"),
    GLAD      = R(VENDOR_DIR .. "/GLAD/include"),
    Box2D     = R(VENDOR_DIR .. "/Box2D/include"),
    json      = R(VENDOR_DIR .. "/nlohmannjson/include"),
    ImGui     = R(VENDOR_DIR .. "/imgui"),
    glm       = R(VENDOR_DIR .. "/glm"),
    entt      = R(VENDOR_DIR .. "/entt/include"),
    stb_image = R(VENDOR_DIR .. "/stb_image"),
    yaml_cpp  = R(VENDOR_DIR .. "/yaml-cpp/include"),
    ImGuizmo  = R(VENDOR_DIR .. "/ImGuizmo"),
    enkiTS    = R(VENDOR_DIR .. "/enkiTS/src"),
    tiny_gltf = R(VENDOR_DIR .. "/tiny_gltf"),
    ImGuizmo  = R(VENDOR_DIR .. "/ImGuizmo"),
    lz4       = R(VENDOR_DIR .. "/lz4"),

    vcpkg     = R(VCPKG_INSTALLED .. "/include"),
    curl      = R(VCPKG_INSTALLED .. "/include"),

   

    VulkanSDK   = VULKAN_SDK .. "/Include",
    shaderc     = VULKAN_SDK .. "/Include/shaderc",
    SPIRV_Cross = VULKAN_SDK .. "/Include/spirv_cross",
    Sandbox     = R("Sandbox/source")
}


LibraryDir = {
    VulkanSDK = VULKAN_SDK .. "/Lib",
    vcpkg     = R(VCPKG_INSTALLED .. "/lib")
}


Library = {
    OpenGL = "opengl32.lib",
    Vulkan = "vulkan-1.lib",

    shaderc_Debug            = "shaderc_sharedd.lib",
    spirv_cross_core_Debug   = "spirv-cross-cored.lib",
    spirv_cross_glsl_Debug   = "spirv-cross-glsld.lib",
    spirv_tools_Debug        = "SPIRV-Toolsd.lib",

    shaderc_Release          = "shaderc_shared.lib",
    spirv_cross_core_Release = "spirv-cross-core.lib",
    spirv_cross_glsl_Release = "spirv-cross-glsl.lib",
    spirv_tools_Release      = "SPIRV-Tools.lib",

    libcurl = "libcurl",
    zlib    = "zlib"
}
