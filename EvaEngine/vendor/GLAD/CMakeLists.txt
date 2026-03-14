# GLAD library
cmake_minimum_required(VERSION 3.16)

project(GLAD)

# Collect GLAD source files
set(GLAD_SOURCES
    src/glad.c
)

# Create static library
add_library(GLAD STATIC ${GLAD_SOURCES})

# Include directories for GLAD
target_include_directories(GLAD PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)
