cmake_minimum_required(VERSION 3.21)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_VISIBILITY_PRESET hidden)

# Geode.dll uses the release C runtime: a Debug build on Windows crashes on strings made by Geode
if (WIN32 AND CMAKE_BUILD_TYPE STREQUAL "Debug")
    message(WARNING "Debug builds crash with Geode on Windows, building RelWithDebInfo instead")
    set(CMAKE_BUILD_TYPE RelWithDebInfo CACHE STRING "Build type" FORCE)
endif()

project(miside-menu VERSION 0.1.0)

file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS src/*.cpp)
add_library(${PROJECT_NAME} SHARED ${SOURCES})

# The menu items are written in Russian in the sources
if (MSVC)
    target_compile_options(${PROJECT_NAME} PRIVATE /utf-8)
endif()

if (NOT DEFINED ENV{GEODE_SDK})
    message(FATAL_ERROR "Unable to find Geode SDK! Please define GEODE_SDK environment variable to point to Geode")
else()
    message(STATUS "Found Geode: $ENV{GEODE_SDK}")
endif()

add_subdirectory($ENV{GEODE_SDK} ${CMAKE_CURRENT_BINARY_DIR}/geode)

setup_geode_mod(${PROJECT_NAME})

# Mita's atlases are decoded off the main thread with Windows' PNG decoder (WIC)
target_link_libraries(${PROJECT_NAME} windowscodecs ole32)
