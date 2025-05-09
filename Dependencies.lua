
VULKAN_SDK = "C:/VulkanSDK/1.4.304.1"
ROOT_DIR = path.getabsolute(os.getcwd()) 

IncludeDir = {}
IncludeDir["GLFW"] = "EvaEngine/vendor/GLFW/include"
IncludeDir["GLAD"] = "EvaEngine/vendor/GLAD/include"
IncludeDir["curl"] = "EvaEngine/vendor/curl/include"
IncludeDir["json"] = "EvaEngine/vendor/nlohmannjson/include"
IncludeDir["ImGui"] = "EvaEngine/vendor/imgui"
IncludeDir["glm"] = "EvaEngine/vendor/glm"
IncludeDir["entt"] = "EvaEngine/vendor/entt/include"
IncludeDir["stb_image"] = "EvaEngine/vendor/stb_image"
IncludeDir["yaml_cpp"] = "EvaEngine/vendor/yaml-cpp/include"
IncludeDir["ImGuizmo"] = "EvaEngine/vendor/ImGuizmo"
IncludeDir["VulkanSDK"] = "%{VULKAN_SDK}/Include"
IncludeDir["shaderc"] = "%{VULKAN_SDK}/Include/shaderc"
IncludeDir["SPIRV_Cross"] = "%{VULKAN_SDK}/Include/spirv_cross"
IncludeDir["Box2D"] =  "EvaEngine/vendor/Box2D/include"
IncludeDir["Sandbox"] = ROOT_DIR .. "/Sandbox/source"
IncludeDir["enkiTS"] = "EvaEngine/vendor/enkiTS/src"

-- Organize library directories
LibraryDir = {}
LibraryDir["VulkanSDK"] = "%{VULKAN_SDK}/Lib"
LibraryDir["Box2D"] = "EvaEngine/vendor/Box2D/bin/%{cfg.buildcfg}-%{cfg.system}-%{cfg.architecture}/Box2D"

-- Organize libraries with separate Debug and Release versions
Library = {}
-- Common libraries
Library["GLFW"] = "GLFW"
Library["GLAD"] = "GLAD"
Library["curl"] = "curl"

Library["ImGui"] = "imgui"
Library["yaml_cpp"] = "yaml-cpp"
Library["OpenGL"] = "opengl32.lib"
Library["Vulkan"] = "vulkan-1.lib"
Library["Box2D"] = "box2dd.lib"

-- Debug specific libraries
Library["shaderc_Debug"] = "shaderc_sharedd.lib"
Library["spirv_cross_core_Debug"] = "spirv-cross-cored.lib"
Library["spirv_cross_glsl_Debug"] = "spirv-cross-glsld.lib"
Library["spirv_tools_Debug"] = "SPIRV-Toolsd.lib"

-- Release specific libraries
Library["shaderc_Release"] = "shaderc_shared.lib"
Library["spirv_cross_core_Release"] = "spirv-cross-core.lib"
Library["spirv_cross_glsl_Release"] = "spirv-cross-glsl.lib"
Library["spirv_tools_Release"] = "SPIRV-Tools.lib"
