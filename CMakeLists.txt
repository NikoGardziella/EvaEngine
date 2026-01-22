cmake_minimum_required(VERSION 3.15)
project(EvaEngine)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Correct include directories
include_directories(
    EvaEngine/source
    Editor/source
    Game/source
    EvaEngine/vendor/spdlog/include
    EvaEngine/vendor/glm
    EvaEngine/vendor/stb
    EvaEngine/vendor/imgui
    EvaEngine/vendor/vulkan/include
    # Add any other vendor includes you have
)

# Find all source files
file(GLOB_RECURSE ENGINE_SOURCES 
    "EvaEngine/source/*.cpp"
)

file(GLOB_RECURSE EDITOR_SOURCES
    "Editor/source/*.cpp"
)

# Create library/executable
add_executable(EvaEngine ${ENGINE_SOURCES} ${EDITOR_SOURCES})