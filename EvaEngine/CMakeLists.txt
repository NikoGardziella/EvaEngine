cmake_minimum_required(VERSION 3.20)
project(EvaEngine LANGUAGES CXX)

add_library(EvaEngine STATIC)

# Precompiled header
target_precompile_headers(EvaEngine PRIVATE source/pch.h)

# Source files
file(GLOB_RECURSE EVAENGINE_SRC
    source/*.cpp
    source/*.h
    vendor/glm/glm/**/*.hpp
    vendor/glm/glm/**/*.inl
    vendor/stb_image/*.cpp
    vendor/stb_image/*.h
    vendor/ImGuizmo/*.cpp
    vendor/ImGuizmo/*.h
    vendor/enkiTS/src/*.cpp
    vendor/enkiTS/src/*.h
    assets/shaders/*
)
target_sources(EvaEngine PRIVATE ${EVAENGINE_SRC})

# Include directories
target_include_directories(EvaEngine PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/source
    ${CMAKE_CURRENT_SOURCE_DIR}/vendor/spdlog/include
    ${CMAKE_CURRENT_SOURCE_DIR}/vendor/vcpkg/x64-windows/include
    ${CMAKE_CURRENT_SOURCE_DIR}/vendor/GLFW/include
    ${CMAKE_CURRENT_SOURCE_DIR}/vendor/GLAD/include
    ${CMAKE_CURRENT_SOURCE_DIR}/vendor/imgui
    ${CMAKE_CURRENT_SOURCE_DIR}/vendor/yaml-cpp/include
    ${CMAKE_CURRENT_SOURCE_DIR}/vendor/Box2D/include
    ${CMAKE_CURRENT_SOURCE_DIR}/vendor/glm
    ${CMAKE_CURRENT_SOURCE_DIR}/vendor/stb_image
    ${CMAKE_CURRENT_SOURCE_DIR}/vendor/enkiTS/src
    ${CMAKE_CURRENT_SOURCE_DIR}/vendor/ImGuizmo
    ${VULKAN_INCLUDE_DIRS}
    ${SHADERC_INCLUDE_DIR}
    ${SPIRV_CROSS_INCLUDE_DIR}
)

# Defines
target_compile_definitions(EvaEngine PRIVATE
    _CRT_SECURE_NO_WARNINGS
    GLFW_INCLUDE_NONE
    YAML_CPP_STATIC_DEFINE
)

if(WIN32)
    target_compile_definitions(EvaEngine PRIVATE
        EE_PLATFORM_WINDOWS
        EE_BUILD_DLL
        SPDLOG_NO_UNICODE
    )
endif()

# Vendor dependencies
add_subdirectory(vendor/GLFW)
add_subdirectory(vendor/GLAD)
add_subdirectory(vendor/imgui)
add_subdirectory(vendor/yaml-cpp)
add_subdirectory(vendor/Box2D)

# Base engine links
target_link_libraries(EvaEngine PUBLIC
    glfw
    glad
    imgui
    yaml-cpp
    Box2D
    ${VULKAN_LIBRARIES}
)

# Optional Shader-related libraries
if(SHADERC_LIBRARY_RELEASE OR SHADERC_LIBRARY_DEBUG)
    target_link_libraries(EvaEngine PUBLIC
        $<$<CONFIG:Debug>:${SHADERC_LIBRARY_DEBUG}>
        $<$<CONFIG:Release>:${SHADERC_LIBRARY_RELEASE}>
        $<$<CONFIG:Dist>:${SHADERC_LIBRARY_RELEASE}>
    )
endif()

if(SPIRV_CROSS_CORE)
    target_link_libraries(EvaEngine PUBLIC
        ${SPIRV_CROSS_CORE}
        ${SPIRV_CROSS_GLSL}
    )
endif()

# Per-configuration defines and optimization flags
foreach(CONFIG_TYPE IN ITEMS Debug Release Dist)
    if(CONFIG_TYPE STREQUAL "Debug")
        target_compile_definitions(EvaEngine PRIVATE EE_DEBUG)
        target_compile_options(EvaEngine PRIVATE $<$<CONFIG:Debug>:/Zi>)
    elseif(CONFIG_TYPE STREQUAL "Release")
        target_compile_definitions(EvaEngine PRIVATE EE_RELEASE)
        target_compile_options(EvaEngine PRIVATE $<$<CONFIG:Release>:/O2>)
    elseif(CONFIG_TYPE STREQUAL "Dist")
        target_compile_definitions(EvaEngine PRIVATE EE_DIST)
        target_compile_options(EvaEngine PRIVATE $<$<CONFIG:Dist>:/O2>)
    endif()
endforeach()
