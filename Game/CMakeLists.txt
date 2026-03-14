cmake_minimum_required(VERSION 3.20)
project(Game LANGUAGES CXX)

add_executable(Game)

# Sources
file(GLOB_RECURSE GAME_SRC
    source/*.cpp
    source/*.h
)
target_sources(Game PRIVATE ${GAME_SRC})

# Include EvaEngine headers
target_include_directories(Game PRIVATE
    ${CMAKE_SOURCE_DIR}/EvaEngine/source
)

# Link EvaEngine
target_link_libraries(Game PRIVATE EvaEngine)

# Defines
target_compile_definitions(Game PRIVATE EE_GAME)

# Configurations
foreach(CONFIG_TYPE IN ITEMS Debug Release Dist)
    if(CONFIG_TYPE STREQUAL "Debug")
        target_compile_definitions(Game PRIVATE EE_DEBUG)
    elseif(CONFIG_TYPE STREQUAL "Release")
        target_compile_definitions(Game PRIVATE EE_RELEASE)
    elseif(CONFIG_TYPE STREQUAL "Dist")
        target_compile_definitions(Game PRIVATE EE_DIST)
    endif()
endforeach()
