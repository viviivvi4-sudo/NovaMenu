cmake_minimum_required(VERSION 3.21)

# Project name must match mod.json "id" style, use something simple
project(FloatingMenuMod VERSION 1.0.0)

file(GLOB_RECURSE SOURCES
    src/*.cpp
    src/*.hpp
)

add_library(${PROJECT_NAME} SHARED ${SOURCES})

target_include_directories(${PROJECT_NAME} PRIVATE src)

# Pull in Geode SDK. When built through geode-cli / geode-build-action,
# CLI already sets up the SDK, this just links it.
if (NOT DEFINED ENV{GEODE_SDK})
    message(FATAL_ERROR "GEODE_SDK environment variable not set. Install geode-cli first.")
endif()

add_subdirectory($ENV{GEODE_SDK} ${CMAKE_BINARY_DIR}/geode)

setup_geode_mod(${PROJECT_NAME})
