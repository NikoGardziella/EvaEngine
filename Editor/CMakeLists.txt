cmake_minimum_required(VERSION 3.20)
project(Editor LANGUAGES CXX)

add_executable(Editor)

# Sources
file(GLOB_RECURSE EDITOR_SRC
    source/*.cpp
    source/*.h
)
target_sources(Editor PRIVATE ${EDITOR_SRC})

# Include EvaEngine headers
target_include_directories(Editor PRIVATE
    ${CMAKE_SOURCE_DIR}/EvaEngine/source
)

# Link EvaEngine
target_link_libraries(Editor PRIVATE EvaEngine)

# Defines
target_compile_definitions(Editor PRIVATE EE_EDITOR)

# Configurations
foreach(CONFIG_TYPE IN ITEMS Debug Release Dist)
    if(CONFIG_TYPE STREQUAL "Debug")
        target_compile_definitions(Editor PRIVATE EE_DEBUG)
    elseif(CONFIG_TYPE STREQUAL "Release")
        target_compile_definitions(Editor PRIVATE EE_RELEASE)
    elseif(CONFIG_TYPE STREQUAL "Dist")
        target_compile_definitions(Editor PRIVATE EE_DIST)
    endif()
endforeach()
